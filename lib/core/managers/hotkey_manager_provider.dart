import 'package:haliclip/core/models/hotkey_config.dart';
import 'package:haliclip/features/settings/providers/settings_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:hotkey_manager/hotkey_manager.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'package:haliclip/core/managers/window_manager_provider.dart';

part 'hotkey_manager_provider.g.dart';

/// 全局热键管理器。
///
/// 负责在系统底层注册快捷键，以便在任何应用程序中都能通过特定按键组合
/// 快速唤起或切换 HaliClip 的剪贴板历史主窗口。
@Riverpod(keepAlive: true)
class AppHotKeyManager extends _$AppHotKeyManager {
  /// 初始化热键管理器。
  ///
  /// 监听设置中的快捷键配置变化，并实时更新系统级热键注册。
  @override
  FutureOr<void> build() async {
    _setupHotkey();
    ref.listen(hotkeyOpenProvider, (previous, next) {
      _setupHotkey();
    });

    ref.onDispose(() {
      hotKeyManager.unregisterAll();
    });
  }

  /// 设置并注册系统热键。
  ///
  /// 首先注销已有的所有热键以防冲突，然后根据 [config] 注册。
  /// 仅当配置中包含至少一个修饰键时才进行注册，避免误触。
  ///
  /// [config] 快捷键配置对象。
  Future<void> _setupHotkey() async {
    await hotKeyManager.unregisterAll();
    final AppHotKeyConfig config = ref.read(hotkeyOpenProvider);
    // 验证：必须包含至少一个修饰键（Ctrl, Alt, Shift, Meta），不能单独使用主键位
    if (config.modifiers.isEmpty) {
      debugPrint('[HotKeyManager] 取消注册：未检测到修饰键，不允许使用单键作为全局热键');
      return;
    }

    final hotKey = config.toHotKey();

    if (hotKey != null) {
      try {
        await hotKeyManager.register(
          hotKey,
          keyDownHandler: (_) {
            ref
                .read(appWindowManagerProvider.notifier)
                .toggleHistory(source: TriggerSource.hotkey);
          },
        );
        debugPrint('[HotKeyManager] 成功注册热键: ${hotKey.toJson()}');
      } catch (e) {
        debugPrint('[HotKeyManager] 注册热键失败: $e');
      }
    }
  }
}
