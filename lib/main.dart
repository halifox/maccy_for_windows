import 'dart:convert';

import 'package:clipboard/core/managers/window_manager_provider.dart';
import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'app.dart';
import 'features/settings/ui/settings_page.dart';

void main(List<String> args) async {
  WidgetsFlutterBinding.ensureInitialized();

  // 无论哪个 isolate 都需要初始化的部分
  // 注意：次 isolate 不需要重新初始化 window_manager 的主窗口控制，
  // 但需要 WidgetsFlutterBinding。
  print(args);
  if (args.firstOrNull == 'multi_window') {
    // --- 次 Isolate (设置窗口) ---
    final windowId = int.parse(args[1]);
    final Map<String, dynamic> data = jsonDecode(args[2]);

    runApp(
      ProviderScope(
        child: _SecondaryWindowRoot(windowId: windowId, route: data['route'] ?? ''),
      ),
    );
  } else {
    // --- 主 Isolate (主面板 + 监听逻辑) ---
    await AppWindowManager.init();
    runApp(ProviderScope(child: const HaliClipApp()));
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
      theme: ThemeData(fontFamily: '.AppleSystemUIFont', brightness: Brightness.light),
      darkTheme: ThemeData(fontFamily: '.AppleSystemUIFont', brightness: Brightness.dark),
      home: route == 'settings' ? const SettingsPage() : const Scaffold(body: Center(child: Text('Unknown Route'))),
    );
  }
}
