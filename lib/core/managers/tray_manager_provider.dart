import 'dart:io';
import 'package:tray_manager/tray_manager.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'window_manager_provider.dart';

part 'tray_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppTrayManager extends _$AppTrayManager with TrayListener {
  @override
  void build() {
    trayManager.addListener(this);
  }

  Future<void> init() async {
    await trayManager.setIcon('assets/icon.png');
    List<MenuItem> items = [
      MenuItem(
        key: 'show_history',
        label: 'Show History',
      ),
      MenuItem(
        key: 'settings',
        label: 'Settings',
      ),
      MenuItem.separator(),
      MenuItem(
        key: 'exit_app',
        label: 'Exit',
      ),
    ];
    await trayManager.setContextMenu(Menu(items: items));
  }

  @override
  void onTrayIconMouseDown() {
    ref.read(appWindowManagerProvider.notifier).showAtCursor();
  }

  @override
  void onTrayIconRightMouseDown() {
    trayManager.popUpContextMenu();
  }

  @override
  void onTrayMenuItemClick(MenuItem menuItem) {
    if (menuItem.key == 'show_history') {
      ref.read(appWindowManagerProvider.notifier).showAtCursor();
    } else if (menuItem.key == 'exit_app') {
      exit(0);
    }
  }
}