import 'dart:io';

import 'package:clipboard/app.dart';
import 'package:clipboard/features/history/providers/history_providers.dart';
import 'package:clipboard/features/settings/providers/settings_provider.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:screen_retriever/screen_retriever.dart';
import 'package:tray_manager/tray_manager.dart';
import 'package:window_manager/window_manager.dart';

part 'window_manager_provider.g.dart';

/// 触发窗口显示的来源。
enum TriggerSource {
  /// 来自系统托盘点击。
  tray,

  /// 来自全局快捷键。
  hotkey,
}

/// 窗口管理器，负责主窗口（剪贴板历史、设置界面）的显示、隐藏、定位以及生命周期管理。
@Riverpod(keepAlive: true)
class AppWindowManager extends _$AppWindowManager with WindowListener {
  bool _isShowing = false;
  DateTime? _lastHideTime;
  final Size _windowSize = const Size(450, 450);

  /// 初始化窗口管理器，注册监听器。
  @override
  FutureOr<void> build() async {
    debugPrint('🚀 AppWindowManager: 开始初始化...');
    windowManager.addListener(this);
    ref.onDispose(() {
      debugPrint('🛑 AppWindowManager: 正在注销监听器并销毁...');
      windowManager.removeListener(this);
    });
    debugPrint('✅ AppWindowManager: 初始化完成');
  }

  /// 初始化窗口管理插件及默认选项。
  static Future<void> init() async {
    debugPrint('📦 WindowManager 插件: 正在确保初始化...');
    await windowManager.ensureInitialized();

    WindowOptions windowOptions = const WindowOptions(
      size: Size(450, 450),
      center: true,
      backgroundColor: Colors.transparent,
      skipTaskbar: true,
      titleBarStyle: TitleBarStyle.hidden,
      alwaysOnTop: true,
    );

    await windowManager.waitUntilReadyToShow(windowOptions, () async {
      await windowManager.setAsFrameless();
      await windowManager.setHasShadow(true);
      await windowManager.setResizable(false);
      await windowManager.hide();
      debugPrint('✅ WindowManager 插件: 已就绪并隐藏主窗口');
    });
  }

  Future<void> toggleHistory({
    TriggerSource source = TriggerSource.hotkey,
  }) async {
    debugPrint('🪟 AppWindowManager: 切换历史窗口状态 (来源: $source)...');
    final now = DateTime.now();
    // 如果刚刚（200ms内）因为失焦而隐藏，说明可能是点击托盘导致的
    // 此时我们不应该再次显示它
    if (_lastHideTime != null &&
        now.difference(_lastHideTime!) < const Duration(milliseconds: 200)) {
      debugPrint('🪟 AppWindowManager: 触发过快，忽略此次切换请求');
      return;
    }
    if (_isShowing) {
      hideHistory();
    } else {
      showHistory(source: source);
    }
  }

  /// 显示剪贴板历史窗口。
  /// 会根据触发来源（托盘或快捷键/设置）自动调整窗口位置。
  Future<void> showHistory({
    TriggerSource source = TriggerSource.hotkey,
  }) async {
    debugPrint('🪟 AppWindowManager: 准备显示历史窗口...');

    if (Platform.isMacOS || Platform.isWindows) {
      try {
        const platform = MethodChannel('com.hali.clip/native_utils');
        await platform.invokeMethod('recordActiveApp');
      } catch (e) {
        debugPrint('⚠️ AppWindowManager: 记录活跃应用失败: $e');
      }
    }

    router.go('/');
    await windowManager.setSize(_windowSize);

    if (source == TriggerSource.tray) {
      await _positionNearTray();
    } else {
      final popupPosition = ref.read(popupPositionProvider);
      debugPrint('🪟 AppWindowManager: 配置显示位置为: $popupPosition');
      if (popupPosition == 'cursor') {
        await _positionNearCursor();
      } else {
        debugPrint('🪟 AppWindowManager: 将窗口居中显示');
        await windowManager.center();
      }
    }

    await windowManager.show();
    await windowManager.focus();
    _isShowing = true;
    debugPrint('🪟 AppWindowManager: 窗口已显示并聚焦');

    // 重置搜索内容和选中项
    ref.read(historySearchQueryProvider.notifier).set('');
    ref.read(historySelectedIndexProvider.notifier).set(0);
    ref.read(historyFocusRequestProvider.notifier).request();
  }

  /// 计算并将窗口定位在当前鼠标光标附近。
  Future<void> _positionNearCursor() async {
    debugPrint('🪟 AppWindowManager: 正在计算光标附近位置...');
    Offset cursorPoint = await screenRetriever.getCursorScreenPoint();
    List<Display> displays = await screenRetriever.getAllDisplays();
    Display targetDisplay = displays.first;

    for (var display in displays) {
      final rect = Rect.fromLTWH(
        display.visiblePosition?.dx ?? 0,
        display.visiblePosition?.dy ?? 0,
        display.visibleSize?.width ?? display.size.width,
        display.visibleSize?.height ?? display.size.height,
      );
      if (rect.contains(cursorPoint)) {
        targetDisplay = display;
        break;
      }
    }

    final visiblePos = targetDisplay.visiblePosition ?? Offset.zero;
    final visibleSize = targetDisplay.visibleSize ?? targetDisplay.size;

    // 默认在光标右下方显示，但如果超出屏幕，则调整位置
    double x = cursorPoint.dx;
    double y = cursorPoint.dy;

    // 边界检查
    if (x + _windowSize.width > visiblePos.dx + visibleSize.width) {
      x = x - _windowSize.width;
    }
    if (y + _windowSize.height > visiblePos.dy + visibleSize.height) {
      y = y - _windowSize.height;
    }

    // 确保不小于可见区域起始位置
    x = x.clamp(
      visiblePos.dx,
      visiblePos.dx + visibleSize.width - _windowSize.width,
    );
    y = y.clamp(
      visiblePos.dy,
      visiblePos.dy + visibleSize.height - _windowSize.height,
    );

    debugPrint('🪟 AppWindowManager: 设置窗口位置到光标附近 ($x, $y)');
    await windowManager.setPosition(Offset(x, y));
  }

  /// 计算并将窗口定位在系统托盘图标附近。
  Future<void> _positionNearTray() async {
    debugPrint('🪟 AppWindowManager: 正在计算托盘附近位置...');
    Rect? trayBounds = await trayManager.getBounds();

    // 如果无法获取托盘位置，则回退到光标位置
    if (trayBounds == null) {
      debugPrint('🪟 AppWindowManager: 无法获取托盘边界，回退到光标位置');
      await _positionNearCursor();
      return;
    }

    List<Display> displays = await screenRetriever.getAllDisplays();
    Display targetDisplay = displays.first;
    for (var display in displays) {
      final rect = Rect.fromLTWH(
        display.visiblePosition?.dx ?? 0,
        display.visiblePosition?.dy ?? 0,
        display.visibleSize?.width ?? display.size.width,
        display.visibleSize?.height ?? display.size.height,
      );
      if (rect.contains(trayBounds.center)) {
        targetDisplay = display;
        break;
      }
    }

    final visiblePos = targetDisplay.visiblePosition ?? Offset.zero;
    final visibleSize = targetDisplay.visibleSize ?? targetDisplay.size;
    final screenWidth = targetDisplay.size.width;
    final screenHeight = targetDisplay.size.height;
    final screenX = targetDisplay.visiblePosition?.dx ?? 0;
    final screenY = targetDisplay.visiblePosition?.dy ?? 0;

    double x = trayBounds.center.dx - _windowSize.width / 2;
    double y = trayBounds.center.dy - _windowSize.height / 2;

    // 常见的任务栏位置：底部、顶部、左侧、右侧
    if (trayBounds.top > screenY + screenHeight * 0.8) {
      // 任务栏在底部
      y = trayBounds.top - _windowSize.height - 10;
    } else if (trayBounds.bottom < screenY + screenHeight * 0.2) {
      // 任务栏在顶部
      y = trayBounds.bottom + 10;
    } else if (trayBounds.left > screenX + screenWidth * 0.8) {
      // 任务栏在右侧
      x = trayBounds.left - _windowSize.width - 10;
      y = trayBounds.center.dy - _windowSize.height / 2;
    } else if (trayBounds.right < screenX + screenWidth * 0.2) {
      // 任务栏在左侧
      x = trayBounds.right + 10;
      y = trayBounds.center.dy - _windowSize.height / 2;
    }

    // 最终边界检查
    x = x.clamp(
      visiblePos.dx,
      visiblePos.dx + visibleSize.width - _windowSize.width,
    );
    y = y.clamp(
      visiblePos.dy,
      visiblePos.dy + visibleSize.height - _windowSize.height,
    );

    debugPrint('🪟 AppWindowManager: 设置窗口位置到托盘附近 ($x, $y)');
    await windowManager.setPosition(Offset(x, y));
  }

  /// 隐藏当前窗口，并记录隐藏时间。
  Future<void> hideHistory() async {
    debugPrint('🪟 AppWindowManager: 正在隐藏历史窗口...');
    router.go('/');
    await windowManager.hide();
    _isShowing = false;
    _lastHideTime = DateTime.now();
  }

  /// 显示设置界面，调整窗口大小并居中显示。
  Future<void> showSettings() async {
    debugPrint('🪟 AppWindowManager: 显示设置窗口...');
    router.go('/settings');
    await windowManager.setSize(const Size(800, 800));
    await windowManager.center();
    await windowManager.show();
    await windowManager.focus();
    _isShowing = true;
  }

  /// 当窗口失去焦点时，自动隐藏剪贴板历史窗口。
  @override
  void onWindowBlur() {
    debugPrint('🪟 AppWindowManager: 窗口失去焦点，执行自动隐藏');
    hideHistory();
  }

  /// 窗口事件通用回调。
  @override
  void onWindowEvent(String eventName) {
    // debugPrint('🪟 Window Event: $eventName');
  }

  @override
  void onWindowUndocked() => debugPrint('🪟 Window Event: Undocked');
  @override
  void onWindowDocked() => debugPrint('🪟 Window Event: Docked');
  @override
  void onWindowLeaveFullScreen() => debugPrint('🪟 Window Event: LeaveFullScreen');
  @override
  void onWindowEnterFullScreen() => debugPrint('🪟 Window Event: EnterFullScreen');
  @override
  void onWindowMoved() => debugPrint('🪟 Window Event: Moved');
  @override
  void onWindowMove() => debugPrint('🪟 Window Event: Move');
  @override
  void onWindowResized() => debugPrint('🪟 Window Event: Resized');
  @override
  void onWindowResize() => debugPrint('🪟 Window Event: Resize');
  @override
  void onWindowRestore() => debugPrint('🪟 Window Event: Restore');
  @override
  void onWindowMinimize() => debugPrint('🪟 Window Event: Minimize');
  @override
  void onWindowUnmaximize() => debugPrint('🪟 Window Event: Unmaximize');
  @override
  void onWindowMaximize() => debugPrint('🪟 Window Event: Maximize');
  @override
  void onWindowFocus() => debugPrint('🪟 Window Event: Focus');
  @override
  void onWindowClose() => debugPrint('🪟 Window Event: Close');
}
