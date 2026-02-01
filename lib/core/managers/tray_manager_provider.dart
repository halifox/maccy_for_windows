import 'dart:io';
import 'package:tray_manager/tray_manager.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'window_manager_provider.dart';
import '../../app.dart';

part 'tray_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppTrayManager extends _$AppTrayManager with TrayListener {
  @override
  void build() {
    trayManager.addListener(this);
  }

  Future<void> init() async {
    await trayManager.setIcon(
      Platform.isMacOS ? 'assets/icon.png' : 'assets/icon.png', // 可以根据平台选择不同文件
      isTemplate: true, // 关键：设置为模板图标，macOS 会自动处理反色
    );
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
    // 左键单击：切换显示或隐藏
    ref.read(appWindowManagerProvider.notifier).toggle();
  }

  @override
  void onTrayIconRightMouseDown() {
    // 右键单击逻辑：显示上下文菜单
    trayManager.popUpContextMenu();
  }

  @override
  void onTrayMenuItemClick(MenuItem menuItem) {
    if (menuItem.key == 'show_history') {
      router.go('/');
      ref.read(appWindowManagerProvider.notifier).showAtCursor();
    } else if (menuItem.key == 'settings') {
      router.go('/settings');
      ref.read(appWindowManagerProvider.notifier).showSettings();
    } else if (menuItem.key == 'exit_app') {
      exit(0);
    }
  }
}
