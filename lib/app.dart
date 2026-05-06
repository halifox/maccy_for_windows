import 'package:maccy/core/managers/initialization_provider.dart';
import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'package:maccy/features/history/ui/history_page.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:maccy/features/settings/ui/settings_page.dart';

/// 根导航器 Key。
final _rootNavigatorKey = GlobalKey<NavigatorState>();

/// 应用程序路由配置。
///
/// 定义了根路径（历史记录页）和设置路径（设置页）。
final router = GoRouter(
  navigatorKey: _rootNavigatorKey,
  initialLocation: '/',
  routes: [
    GoRoute(path: '/', builder: (context, state) => const BlankPage()),
    GoRoute(path: '/clipboard', builder: (context, state) => const HistoryPage()),
    GoRoute(
      path: '/settings',
      builder: (context, state) => const SettingsPage(),
    ),
  ],
);

/// Maccy 应用程序主类。
///
/// 作为整个应用的入口 Widget，负责监听应用启动状态（startup）和主题模式（themeMode），
/// 并根据状态渲染主应用、闪屏页或错误页。
class MaccyApp extends HookConsumerWidget {
  const MaccyApp({super.key});

  /// 构建应用 UI。
  ///
  /// [context] 构建上下文。
  /// [ref] Riverpod 引用。
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final startup = ref.watch(appStartupProvider);
    final themeMode = ref.watch(themeModeProvider);
    return startup.when(
      data: (_) => MaterialApp.router(
        title: 'Maccy',
        debugShowCheckedModeBanner: false,
        themeMode: switch (themeMode) {
          'light' => ThemeMode.light,
          'dark' => ThemeMode.dark,
          'system' => ThemeMode.system,
          String() => throw UnimplementedError(),
        },
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: Colors.blue,
            brightness: Brightness.light,
          ),
          useMaterial3: true,
        ),
        darkTheme: ThemeData(
          colorScheme: ColorScheme.fromSeed(
            seedColor: Colors.blue,
            brightness: Brightness.dark,
          ),
          useMaterial3: true,
        ),
        routerConfig: router,
      ),
      loading: () => const SplashScreen(),
      error: (e, st) => ErrorScreen(e),
    );
  }
}

/// 空白页面。
///
/// 当应用程序窗口隐藏时切换至此页面，以卸载其他功能页面的 Widget 树并尽可能释放内存资源。
class BlankPage extends StatelessWidget {
  const BlankPage({super.key});

  @override
  Widget build(BuildContext context) {
    return const Scaffold(backgroundColor: Colors.transparent, body: SizedBox.shrink());
  }
}

/// 闪屏页面。
///
/// 在应用程序进行核心服务（数据库、热键、托盘等）初始化时显示。
class SplashScreen extends StatelessWidget {
  const SplashScreen({super.key});

  /// 构建闪屏页 UI。
  ///
  /// [context] 构建上下文。
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
                '正在初始化 Maccy...',
                style: Theme.of(
                  context,
                ).textTheme.bodyMedium?.copyWith(color: Colors.grey),
              ),
            ],
          ),
        ),
      ),
    );
  }
}

/// 错误显示页面。
///
/// 当应用初始化失败时显示，允许用户查看错误信息并重试初始化。
///
/// 字段说明:
/// [e] 发生的异常对象。
class ErrorScreen extends ConsumerWidget {

  const ErrorScreen(this.e, {super.key});
  final Object e;

  /// 构建错误页 UI。
  ///
  /// [context] 构建上下文。
  /// [ref] Riverpod 引用。
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
                const Icon(
                  Icons.error_outline,
                  size: 64,
                  color: Colors.redAccent,
                ),
                const SizedBox(height: 24),
                Text('启动失败', style: Theme.of(context).textTheme.headlineSmall),
                const SizedBox(height: 12),
                Text(
                  e.toString(),
                  textAlign: TextAlign.center,
                  style: Theme.of(
                    context,
                  ).textTheme.bodyMedium?.copyWith(color: Colors.grey[600]),
                ),
                const SizedBox(height: 32),
                ElevatedButton.icon(
                  onPressed: () {
                    debugPrint('[App] 用户触发重试应用初始化');
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
