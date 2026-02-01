import 'dart:convert';
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:window_manager/window_manager.dart';
import 'package:screen_retriever/screen_retriever.dart';
import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'package:tray_manager/tray_manager.dart'; // 引入以切换图标
import '../../features/settings/providers/settings_provider.dart';

part 'window_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppWindowManager extends _$AppWindowManager with WindowListener {
  @override
  void build() {
    windowManager.addListener(this);
  }

  @override
  void onWindowBlur() {
    // 关键：点击其他托盘或屏幕任何位置，都会触发此回调
    hide();
  }

  /// 切换显示状态并处理托盘图标的高亮模拟
  Future<void> toggle() async {
    bool isVisible = await windowManager.isVisible();
    if (isVisible) {
      await hide();
    } else {
      await showAtCursor();
    }
  }

  Future<void> showAtCursor() async {
    Offset cursorPosition = await screenRetriever.getCursorScreenPoint();
    
    final settings = await ref.read(settingsProvider.future);
    final Size windowSize = Size(settings.windowWidth, 450);

    await windowManager.setSize(windowSize);
    // 稍微向左偏移，使窗口中心大致对准图标
    await windowManager.setPosition(Offset(cursorPosition.dx - windowSize.width / 2, cursorPosition.dy));

    await windowManager.setAsFrameless();
    await windowManager.setHasShadow(true);
    await windowManager.setAlwaysOnTop(true);
    
    // 显示并强制获取焦点，这是“点击其他位置隐藏”的前提
    await windowManager.show();
    await windowManager.focus();

    // 模拟 macOS 托盘按下效果（如果插件支持，这里可以切换一个带背景色的图标）
    // await trayManager.setIcon('assets/icon_active.png', isTemplate: true);
  }

  Future<void> showSettings() async {
    final window = await DesktopMultiWindow.createWindow(jsonEncode({
      'route': 'settings',
    }));
    
    window
      ..setFrame(const Offset(100, 100) & const Size(800, 600))
      ..center()
      ..setTitle('HaliClip Settings')
      ..show();
  }

  Future<void> hide() async {
    await windowManager.hide();
    // 恢复托盘普通状态
    // await trayManager.setIcon('assets/icon.png', isTemplate: true);
  }
}