import 'package:flutter/material.dart';
import 'package:go_router/go_router.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'features/history/ui/history_page.dart';
import 'features/settings/ui/settings_page.dart';

import 'features/settings/providers/settings_provider.dart';

final _rootNavigatorKey = GlobalKey<NavigatorState>();

final router = GoRouter(
  navigatorKey: _rootNavigatorKey,
  initialLocation: '/',
  routes: [
    GoRoute(
      path: '/',
      builder: (context, state) => const HistoryPage(),
    ),
    GoRoute(
      path: '/settings',
      builder: (context, state) => const SettingsPage(),
    ),
  ],
);

class HaliClipApp extends HookConsumerWidget {
  const HaliClipApp({super.key});

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final settingsAsync = ref.watch(settingsProvider);
    
    final themeMode = settingsAsync.maybeWhen(
      data: (s) {
        if (s.themeMode == 'light') return ThemeMode.light;
        if (s.themeMode == 'dark') return ThemeMode.dark;
        return ThemeMode.system;
      },
      orElse: () => ThemeMode.system,
    );

    return MaterialApp.router(
      title: 'HaliClip',
      debugShowCheckedModeBanner: false,
      themeMode: themeMode,
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
    );
  }
}
