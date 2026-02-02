import 'dart:convert';

import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:flutter/material.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:window_manager/window_manager.dart';

part 'window_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppWindowManager extends _$AppWindowManager with WindowListener {
  bool _isShowing = false;
  DateTime? _lastHideTime;

  @override
  void build() {
    print("AppWindowManager");
    windowManager.addListener(this);
    ref.onDispose(() => windowManager.removeListener(this));
  }

  @override
  void onWindowBlur() {
    hideHistory();
  }

  /// 切换显示状态并处理托盘图标的高亮模拟
  Future<void> toggleHistory() async {
    final now = DateTime.now();
    // 如果刚刚（200ms内）因为失焦而隐藏，说明可能是点击托盘导致的
    // 此时我们不应该再次显示它
    if (_lastHideTime != null && now.difference(_lastHideTime!) < const Duration(milliseconds: 200)) {
      return;
    }
    if (_isShowing) {
      hideHistory();
    } else {
      showHistory();
    }
  }

  static Future<void> init() async {
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
      await windowManager.setSize(Size(450, 450));
      await windowManager.setPosition(Offset(100, 100));
      await windowManager.setAsFrameless();
      await windowManager.setHasShadow(true);
      await windowManager.setAlwaysOnTop(true);
      await windowManager.setResizable(false);
      await windowManager.show();
      await windowManager.focus();
    });
  }

  Future<void> showHistory() async {
    await windowManager.setSize(Size(450, 450));
    await windowManager.setPosition(Offset(100, 100));
    await windowManager.setAsFrameless();
    await windowManager.setHasShadow(true);
    await windowManager.setAlwaysOnTop(true);
    await windowManager.setResizable(false);
    await windowManager.show();
    await windowManager.focus();
    _isShowing = true;
  }

  Future<void> hideHistory() async {
    await windowManager.minimize();
    await windowManager.hide();
    _isShowing = false;
    _lastHideTime = DateTime.now();
  }

  int? _settingsWindowId;

  Future<void> showSettings() async {
    hideHistory();
    // 检查窗口是否真的还存在（防止用户点关闭按钮销毁了窗口）
    final windowIds = await DesktopMultiWindow.getAllSubWindowIds();
    if (windowIds.contains(_settingsWindowId)) {
      // 窗口存在，直接显示并聚焦
      final windowController = WindowController.fromWindowId(_settingsWindowId!);
      await windowController.show();
      return;
    }

    final window = await DesktopMultiWindow.createWindow(jsonEncode({'route': 'settings'}));
    _settingsWindowId = window.windowId; // 记录新窗口 ID
    window
      ..setFrame(const Offset(100, 100) & const Size(800 * 2, 600 * 2))
      ..center()
      ..setTitle('HaliClip Settings')
      ..show();
  }
}
