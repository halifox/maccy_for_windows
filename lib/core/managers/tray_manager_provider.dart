import 'dart:io';

import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:tray_manager/tray_manager.dart';

import 'window_manager_provider.dart';

part 'tray_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppTrayManager extends _$AppTrayManager with TrayListener {
  @override
  void build() async {
    print("AppTrayManager");
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
    ref.onDispose(() => trayManager.removeListener(this));
  }

  @override
  void onTrayIconMouseDown() {
    // 左键单击：切换显示或隐藏
    ref.read(appWindowManagerProvider.notifier).toggleHistory();
  }
}
