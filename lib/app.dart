import 'package:clipboard/core/managers/initialization_provider.dart';
import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'features/history/ui/history_page.dart';
import 'features/settings/providers/settings_provider.dart';
import 'features/settings/ui/settings_page.dart';

/// 根导航器 Key
final _rootNavigatorKey = GlobalKey<NavigatorState>();

/// 应用程序路由配置
final router = GoRouter(
  navigatorKey: _rootNavigatorKey,
  initialLocation: '/',
  routes: [
    GoRoute(path: '/', builder: (context, state) => const HistoryPage()),
    GoRoute(path: '/settings', builder: (context, state) => const SettingsPage()),
  ],
);

/// HaliClip 应用程序主类
class HaliClipApp extends HookConsumerWidget {
  /// 构造函数
  const HaliClipApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final startup = ref.watch(appStartupProvider);
    final themeMode = ref.watch(themeModeProvider);
    return startup.when(
      data: (_) => MaterialApp.router(
        title: 'HaliClip',
        debugShowCheckedModeBanner: false,
        themeMode: switch (themeMode) {
          'light' => ThemeMode.light,
          'dark' => ThemeMode.dark,
          'system' => ThemeMode.system,
          String() => throw UnimplementedError(),
        },
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue, brightness: Brightness.light),
          useMaterial3: true,
        ),
        darkTheme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.blue, brightness: Brightness.dark),
          useMaterial3: true,
        ),
        routerConfig: router,
      ),
      loading: () => SplashScreen(),
      error: (e, st) => ErrorScreen(e),
    );
  }
}

/// 闪屏页面，用于初始化加载时显示
class SplashScreen extends StatelessWidget {
  const SplashScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: Scaffold(
        body: Center(
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              const SizedBox(
                width: 64,
                height: 64,
                child: CircularProgressIndicator(strokeWidth: 3),
              ),
              const SizedBox(height: 24),
              Text(
                '正在初始化 HaliClip...',
                style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                      color: Colors.grey,
                    ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

/// 错误显示页面
class ErrorScreen extends ConsumerWidget {
  final Object e;

  const ErrorScreen(this.e, {super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      home: Scaffold(
        body: Center(
          child: Padding(
            padding: const EdgeInsets.all(32.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                const Icon(Icons.error_outline, size: 64, color: Colors.redAccent),
                const SizedBox(height: 24),
                Text(
                  '启动失败',
                  style: Theme.of(context).textTheme.headlineSmall,
                ),
                const SizedBox(height: 12),
                Text(
                  e.toString(),
                  textAlign: TextAlign.center,
                  style: Theme.of(context).textTheme.bodyMedium?.copyWith(
                        color: Colors.grey[600],
                      ),
                ),
                const SizedBox(height: 32),
                ElevatedButton.icon(
                  onPressed: () {
                    debugPrint('🔄 用户点击重试初始化...');
                    ref.invalidate(appStartupProvider);
                  },
                  icon: const Icon(Icons.refresh),
                  label: const Text('重试'),
                ),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
