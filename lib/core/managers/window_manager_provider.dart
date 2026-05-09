import 'dart:io';

import 'package:maccy/app.dart';
import 'package:maccy/core/services/screen_service.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:screen_retriever/screen_retriever.dart';
import 'package:shared_preferences/shared_preferences.dart';
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

    // 使用 ScreenService 计算窗口位置
    final Offset position;
    if (source == TriggerSource.tray) {
      position = await ScreenService.calculateWindowPosition(
        position: PopupPosition.statusItem,
        screenIndex: ref.read(popupScreenProvider),
        windowSize: _windowSize,
      );
    } else {
      final positionMode = _parsePopupPosition(ref.read(popupPositionProvider));
      final prefs = ref.read(sharedPrefsProvider);
      final lastPosition = _getLastPosition(prefs);

      position = await ScreenService.calculateWindowPosition(
        position: positionMode,
        screenIndex: ref.read(popupScreenProvider),
        windowSize: _windowSize,
        lastPosition: lastPosition,
      );

      // 保存位置（用于 lastPosition 模式）
      if (positionMode == PopupPosition.lastPosition) {
        await _saveLastPosition(prefs, position);
      }
    }

    await windowManager.setPosition(position);
    await windowManager.show();
    await windowManager.focus();
    _isShowing = true;

    ref.read(historySearchQueryProvider.notifier).set('');
    ref.read(historySelectedIndexProvider.notifier).set(0);
    ref.read(historyFocusRequestProvider.notifier).request();
    debugPrint('[WindowManager] 历史记录窗口已显示');
  }

  /// 解析字符串配置为 PopupPosition 枚举。
  PopupPosition _parsePopupPosition(String value) {
    switch (value) {
      case 'cursor':
        return PopupPosition.cursor;
      case 'center':
        return PopupPosition.center;
      case 'statusItem':
        return PopupPosition.statusItem;
      case 'lastPosition':
        return PopupPosition.lastPosition;
      default:
        return PopupPosition.cursor;
    }
  }

  /// 获取上次保存的窗口位置。
  Offset? _getLastPosition(SharedPreferences prefs) {
    final x = prefs.getDouble('lastWindowPositionX');
    final y = prefs.getDouble('lastWindowPositionY');

    if (x != null && y != null) {
      return Offset(x, y);
    }
    return null;
  }

  /// 保存当前窗口位置。
  Future<void> _saveLastPosition(SharedPreferences prefs, Offset position) async {
    await prefs.setDouble('lastWindowPositionX', position.dx);
    await prefs.setDouble('lastWindowPositionY', position.dy);
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
