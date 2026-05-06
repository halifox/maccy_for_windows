import 'dart:io';

import 'package:maccy/app.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
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

/// 窗口管理器。
///
/// 负责主窗口（剪贴板历史窗口、设置窗口）的生命周期管理，
/// 包括窗口的创建、显示/隐藏切换、窗口位置计算（如光标跟随或托盘跟随）以及失焦自动隐藏逻辑。
///
/// 字段说明:
/// [_isShowing] 标记当前窗口是否处于显示状态。
/// [_lastHideTime] 上次窗口隐藏的时间戳，用于防止触发抖动。
/// [_windowSize] 历史记录窗口的默认尺寸。
@Riverpod(keepAlive: true)
class AppWindowManager extends _$AppWindowManager with WindowListener {
  bool _isShowing = false;
  DateTime? _lastHideTime;
  final Size _windowSize = const Size(450, 450);

  /// 初始化窗口管理器。
  ///
  /// 注册窗口事件监听器，用于响应失焦隐藏等操作。
  @override
  FutureOr<void> build() async {
    windowManager.addListener(this);
    ref.onDispose(() {
      windowManager.removeListener(this);
    });
    debugPrint('[WindowManager] 服务已就绪');
  }

  /// 插件预初始化静态方法。
  ///
  /// 在 [main] 函数中调用，负责底层 window_manager 插件的初始化、无边框阴影设置
  /// 以及窗口的初始隐藏状态配置。
  static Future<void> init() async {
    await windowManager.ensureInitialized();

    const WindowOptions windowOptions = WindowOptions(
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
    });
  }

  /// 切换历史窗口的显示或隐藏状态。
  ///
  /// 包含防抖逻辑：若 200ms 内刚执行过隐藏，则忽略此次显示请求，以处理某些平台托盘点击与失焦事件的冲突。
  ///
  /// [source] 触发切换的来源。
  Future<void> toggleHistory({
    TriggerSource source = TriggerSource.hotkey,
  }) async {
    final now = DateTime.now();
    if (_lastHideTime != null &&
        now.difference(_lastHideTime!) < const Duration(milliseconds: 200)) {
      return;
    }
    if (_isShowing) {
      hideHistory();
    } else {
      showHistory(source: source);
    }
  }

  /// 显示剪贴板历史窗口。
  ///
  /// 执行显示前的准备工作：路由跳转、Native 层活跃应用记录、根据 [source] 计算窗口弹出的位置。
  ///
  /// [source] 显示来源，决定了窗口是出现在光标处、托盘处还是屏幕中央。
  Future<void> showHistory({
    TriggerSource source = TriggerSource.hotkey,
  }) async {
    if (Platform.isMacOS || Platform.isWindows) {
      try {
        const platform = MethodChannel('com.hali.clip/native_utils');
        await platform.invokeMethod('recordActiveApp');
      } catch (e) {
        debugPrint('[WindowManager] 记录活跃应用失败: $e');
      }
    }

    router.go('/clipboard');
    await windowManager.setSize(_windowSize);

    if (source == TriggerSource.tray) {
      await _positionNearTray();
    } else {
      final popupPosition = ref.read(popupPositionProvider);
      if (popupPosition == 'cursor') {
        await _positionNearCursor();
      } else {
        await windowManager.center();
      }
    }

    await windowManager.show();
    await windowManager.focus();
    _isShowing = true;

    ref.read(historySearchQueryProvider.notifier).set('');
    ref.read(historySelectedIndexProvider.notifier).set(0);
    ref.read(historyFocusRequestProvider.notifier).request();
    debugPrint('[WindowManager] 历史记录窗口已显示');
  }

  /// 计算并将窗口定位在当前鼠标光标附近。
  ///
  /// 包含多显示器边界检查，确保窗口不会超出屏幕边缘。
  Future<void> _positionNearCursor() async {
    final Offset cursorPoint = await screenRetriever.getCursorScreenPoint();
    final List<Display> displays = await screenRetriever.getAllDisplays();
    Display targetDisplay = displays.first;

    for (final display in displays) {
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

    double x = cursorPoint.dx;
    double y = cursorPoint.dy;

    if (x + _windowSize.width > visiblePos.dx + visibleSize.width) {
      x = x - _windowSize.width;
    }
    if (y + _windowSize.height > visiblePos.dy + visibleSize.height) {
      y = y - _windowSize.height;
    }

    x = x.clamp(
      visiblePos.dx,
      visiblePos.dx + visibleSize.width - _windowSize.width,
    );
    y = y.clamp(
      visiblePos.dy,
      visiblePos.dy + visibleSize.height - _windowSize.height,
    );

    await windowManager.setPosition(Offset(x, y));
  }

  /// 计算并将窗口定位在系统托盘图标附近。
  ///
  /// 能够根据任务栏的位置（上、下、左、右）自动调整窗口的对齐方式。
  Future<void> _positionNearTray() async {
    final Rect? trayBounds = await trayManager.getBounds();

    if (trayBounds == null) {
      await _positionNearCursor();
      return;
    }

    final List<Display> displays = await screenRetriever.getAllDisplays();
    Display targetDisplay = displays.first;
    for (final display in displays) {
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

    if (trayBounds.top > screenY + screenHeight * 0.8) {
      y = trayBounds.top - _windowSize.height - 10;
    } else if (trayBounds.bottom < screenY + screenHeight * 0.2) {
      y = trayBounds.bottom + 10;
    } else if (trayBounds.left > screenX + screenWidth * 0.8) {
      x = trayBounds.left - _windowSize.width - 10;
      y = trayBounds.center.dy - _windowSize.height / 2;
    } else if (trayBounds.right < screenX + screenWidth * 0.2) {
      x = trayBounds.right + 10;
      y = trayBounds.center.dy - _windowSize.height / 2;
    }

    x = x.clamp(
      visiblePos.dx,
      visiblePos.dx + visibleSize.width - _windowSize.width,
    );
    y = y.clamp(
      visiblePos.dy,
      visiblePos.dy + visibleSize.height - _windowSize.height,
    );

    await windowManager.setPosition(Offset(x, y));
  }

  /// 隐藏当前主窗口并记录时间戳。
  Future<void> hideHistory() async {
    router.go('/clipboard');
    await windowManager.hide();
    _isShowing = false;
    _lastHideTime = DateTime.now();
  }

  /// 显示应用程序的设置界面。
  ///
  /// 将窗口调整为较大的尺寸并居中显示，切换路由至设置页面。
  Future<void> showSettings() async {
    await windowManager.hide();
    await windowManager.setSize(const Size(800, 800));
    router.go('/settings');
    await windowManager.center();
    await windowManager.show();
    await windowManager.focus();
    _isShowing = true;
  }

  /// 窗口失去焦点时的监听回调。
  ///
  /// 用于实现“点击别处自动隐藏”的剪贴板工具常见行为。
  @override
  void onWindowBlur() {
    debugPrint('[WindowManager] 窗口失去焦点，执行自动隐藏');
    hideHistory();
  }

  @override
  void onWindowEvent(String eventName) {}

  @override
  void onWindowUndocked() {}

  @override
  void onWindowDocked() {}

  @override
  void onWindowLeaveFullScreen() {}

  @override
  void onWindowEnterFullScreen() {}

  @override
  void onWindowMoved() {}

  @override
  void onWindowMove() {}

  @override
  void onWindowResized() {}

  @override
  void onWindowResize() {}

  @override
  void onWindowRestore() {}

  @override
  void onWindowMinimize() {}

  @override
  void onWindowUnmaximize() {}

  @override
  void onWindowMaximize() {}

  @override
  void onWindowFocus() {}

  @override
  void onWindowClose() {}
}
