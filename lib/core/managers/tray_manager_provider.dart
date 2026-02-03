import 'dart:io';

import 'package:flutter/cupertino.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:tray_manager/tray_manager.dart';

import 'window_manager_provider.dart';

part 'tray_manager_provider.g.dart';

/// 系统托盘管理器，负责初始化托盘图标、设置悬停提示以及监听托盘点击事件。
@Riverpod(keepAlive: true)
class AppTrayManager extends _$AppTrayManager with TrayListener {
  /// 初始化托盘图标，根据不同操作系统选择相应格式的图标文件。
  @override
  FutureOr<void> build() async {
    debugPrint('🚀 AppTrayManager: 开始初始化托盘...');
    String iconPath = '';
    if (Platform.isMacOS) {
      iconPath = 'assets/tray_icon_32.png';
    } else if (Platform.isLinux) {
      iconPath = 'assets/tray_icon_32.png';
    } else if (Platform.isWindows) {
      iconPath = 'assets/tray_icon_32.ico';
    }
    
    await trayManager.setIcon(iconPath, isTemplate: true);
    await trayManager.setToolTip("hali clip");
    trayManager.addListener(this);
    
    ref.onDispose(() {
      debugPrint('🛑 AppTrayManager: 正在注销托盘监听器...');
      trayManager.removeListener(this);
    });
    debugPrint('✅ AppTrayManager: 托盘初始化完成');
  }

  /// 托盘图标左键点击回调：切换剪贴板历史窗口的显示或隐藏。
  @override
  void onTrayIconMouseDown() {
    debugPrint('🖱️ Tray: 托盘图标按下 (左键)');
    // 左键单击：切换显示或隐藏
    ref.read(appWindowManagerProvider.notifier).toggleHistory(source: TriggerSource.tray);
  }

  /// 托盘菜单项点击回调。
  @override
  void onTrayMenuItemClick(MenuItem menuItem) {
    debugPrint('🖱️ Tray: 菜单项被点击: ${menuItem.label}');
  }

  /// 托盘图标右键松开回调。
  @override
  void onTrayIconRightMouseUp() {
    debugPrint('🖱️ Tray: 托盘图标右键抬起');
  }

  /// 托盘图标右键按下回调。
  @override
  void onTrayIconRightMouseDown() {
    debugPrint('🖱️ Tray: 托盘图标右键按下');
  }

  /// 托盘图标左键松开回调。
  @override
  void onTrayIconMouseUp() {
    debugPrint('🖱️ Tray: 托盘图标左键抬起');
  }
}
