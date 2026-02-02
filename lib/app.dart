import 'package:clipboard/core/managers/initialization_provider.dart';
import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'features/history/ui/history_page.dart';
import 'features/settings/providers/settings_provider.dart';
import 'features/settings/ui/settings_page.dart';

final _rootNavigatorKey = GlobalKey<NavigatorState>();

final router = GoRouter(
  navigatorKey: _rootNavigatorKey,
  initialLocation: '/',
  routes: [
    GoRoute(path: '/', builder: (context, state) => const HistoryPage()),
    GoRoute(path: '/settings', builder: (context, state) => const SettingsPage()),
  ],
);

class HaliClipApp extends HookConsumerWidget {
  const HaliClipApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final startup = ref.watch(appStartupProvider);
    final settingsAsync = ref.watch(settingsProvider);

    final themeMode = settingsAsync.maybeWhen(
      data: (s) {
        if (s.themeMode == 'light') return ThemeMode.light;
        if (s.themeMode == 'dark') return ThemeMode.dark;
        return ThemeMode.system;
      },
      orElse: () => ThemeMode.system,
    );

    return startup.when(
      data: (_) => MaterialApp.router(
        title: 'HaliClip',
        debugShowCheckedModeBanner: false,
        themeMode: themeMode,
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

class SplashScreen extends StatelessWidget {
  const SplashScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}

class ErrorScreen extends StatelessWidget {
  Object e;

  ErrorScreen(this.e, {super.key});

  @override
  Widget build(BuildContext context) {
    return const Placeholder();
  }
}
