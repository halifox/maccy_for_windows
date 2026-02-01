import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:window_manager/window_manager.dart';
import 'package:desktop_multi_window/desktop_multi_window.dart';
import 'core/managers/tray_manager_provider.dart';
import 'core/managers/hotkey_manager_provider.dart';
import 'core/managers/clipboard_manager_provider.dart';
import 'core/managers/window_manager_provider.dart';
import 'features/settings/ui/settings_page.dart';
import 'app.dart';

void main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();
  
  // 无论哪个 isolate 都需要初始化的部分
  // 注意：次 isolate 不需要重新初始化 window_manager 的主窗口控制，
  // 但需要 WidgetsFlutterBinding。

  if (args.firstOrNull == 'multi_window') {
    // --- 次 Isolate (设置窗口) ---
    final windowId = int.parse(args[1]);
    final Map<String, dynamic> data = jsonDecode(args[2]);
    
    runApp(
      ProviderScope(
        child: _SecondaryWindowRoot(
          windowId: windowId,
          route: data['route'] ?? '',
        ),
      ),
    );
  } else {
    // --- 主 Isolate (主面板 + 监听逻辑) ---
    await windowManager.ensureInitialized();

    WindowOptions windowOptions = const WindowOptions(
      size: Size(350, 450),
      center: true,
      backgroundColor: Colors.transparent,
      skipTaskbar: true,
      titleBarStyle: TitleBarStyle.hidden,
      alwaysOnTop: true,
    );
    
    await windowManager.waitUntilReadyToShow(windowOptions, () async {
      await windowManager.setAsFrameless();
      await windowManager.setHasShadow(true);
      // 确保窗口在 macOS 上具有 Panel 特性
      await windowManager.setResizable(false);
      await windowManager.hide();
    });    final container = ProviderContainer();
    
    // 初始化监听逻辑
    container.read(appWindowManagerProvider); 
    await container.read(appTrayManagerProvider.notifier).init();
    await container.read(appHotKeyManagerProvider.notifier).init();
    container.read(appClipboardManagerProvider.notifier).start();

    runApp(
      UncontrolledProviderScope(
        container: container,
        child: const HaliClipApp(),
      ),
    );
  }
}

/// 设置窗口的根组件，有助于 Flutter 捕获热重载信号
class _SecondaryWindowRoot extends StatelessWidget {
  final int windowId;
  final String route;

  const _SecondaryWindowRoot({required this.windowId, required this.route});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      // 使用 macOS 26 的字体规范
      theme: ThemeData(
        fontFamily: '.AppleSystemUIFont',
        brightness: Brightness.light,
      ),
      darkTheme: ThemeData(
        fontFamily: '.AppleSystemUIFont',
        brightness: Brightness.dark,
      ),
      home: route == 'settings' ? const SettingsPage() : const Scaffold(body: Center(child: Text('Unknown Route'))),
    );
  }
}
