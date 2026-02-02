import 'package:flutter/services.dart';
import 'package:hotkey_manager/hotkey_manager.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'window_manager_provider.dart';

part 'hotkey_manager_provider.g.dart';

@Riverpod(keepAlive: true)
class AppHotKeyManager extends _$AppHotKeyManager {
  @override
  void build() {
    print('AppHotKeyManager');
    hotKeyManager.register(
      HotKey(key: PhysicalKeyboardKey.keyV, modifiers: [HotKeyModifier.alt], scope: HotKeyScope.system),
      keyDownHandler: (hotKey) {
        ref.read(appWindowManagerProvider.notifier).toggleHistory();
      },
    );
    ref.onDispose(() => hotKeyManager.unregisterAll());
  }
}
