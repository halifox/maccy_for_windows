import 'package:flutter/material.dart';
import 'package:hooks_riverpod/hooks_riverpod.dart';
import 'package:window_manager/window_manager.dart';
import 'core/managers/tray_manager_provider.dart';
import 'core/managers/hotkey_manager_provider.dart';
import 'core/managers/clipboard_manager_provider.dart';
import 'core/managers/window_manager_provider.dart';
import 'app.dart';

void main() async {
  WidgetsFlutterBinding.ensureInitialized();
  
  await windowManager.ensureInitialized();

  WindowOptions windowOptions = const WindowOptions(
    size: Size(350, 500),
    center: true,
    backgroundColor: Colors.transparent,
    skipTaskbar: true,
    titleBarStyle: TitleBarStyle.hidden,
    alwaysOnTop: true,
  );
  
  await windowManager.waitUntilReadyToShow(windowOptions, () async {
    await windowManager.hide();
  });

  final container = ProviderContainer();
  
  // Initialize managers
  container.read(appWindowManagerProvider); // Active listener
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
