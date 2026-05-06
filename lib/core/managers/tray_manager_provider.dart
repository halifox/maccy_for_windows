import 'dart:io';

import 'package:flutter/cupertino.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:tray_manager/tray_manager.dart';

import 'package:maccy/core/managers/window_manager_provider.dart';

part 'tray_manager_provider.g.dart';

/// 系统托盘管理器。
///
/// 负责在操作系统的任务栏或状态栏创建图标，设置悬停提示信息，
/// 并处理托盘图标的点击、右键菜单等交互事件。
@Riverpod(keepAlive: true)
class AppTrayManager extends _$AppTrayManager with TrayListener {
  /// 初始化托盘管理器。
  ///
  /// 根据当前运行的操作系统平台选择相应的图标格式（.png 或 .ico），
  /// 设置全局提示文字并注册交互监听器。
  @override
  FutureOr<void> build() async {
    String iconPath = '';
    if (Platform.isMacOS) {
      iconPath = 'assets/tray_icon_32.png';
    } else if (Platform.isLinux) {
      iconPath = 'assets/tray_icon_32.png';
    } else if (Platform.isWindows) {
      iconPath = 'assets/tray_icon_32.ico';
    }

    await trayManager.setIcon(iconPath, isTemplate: true);
    await trayManager.setToolTip('Maccy');
    trayManager.addListener(this);

    ref.onDispose(() {
      trayManager.removeListener(this);
    });
    debugPrint('[TrayManager] 服务已就绪');
  }

  /// 托盘图标按下回调。
  ///
  /// 通常响应左键点击，用于快速切换历史记录窗口的显隐状态。
  @override
  void onTrayIconMouseDown() {
    ref
        .read(appWindowManagerProvider.notifier)
        .toggleHistory(source: TriggerSource.tray);
  }

  /// 托盘菜单项点击回调。
  ///
  /// [menuItem] 被点击的菜单项对象。
  @override
  void onTrayMenuItemClick(MenuItem menuItem) {}

  @override
  void onTrayIconRightMouseUp() {}

  @override
  void onTrayIconRightMouseDown() {}

  @override
  void onTrayIconMouseUp() {}
}
