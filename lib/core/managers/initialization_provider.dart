import 'package:clipboard/core/managers/tray_manager_provider.dart';
import 'package:clipboard/core/managers/window_manager_provider.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'clipboard_manager_provider.dart';
import 'hotkey_manager_provider.dart';

part 'initialization_provider.g.dart';

@riverpod
Future<void> appStartup(Ref ref) async {
  // 监听所有需要自动运行的服务
  ref.watch(appWindowManagerProvider);
  ref.watch(appTrayManagerProvider);
  ref.watch(appHotKeyManagerProvider);
  ref.watch(appClipboardManagerProvider);
}
