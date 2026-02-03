import 'package:clipboard/core/models/hotkey_config.dart';
import 'package:clipboard/features/settings/providers/settings_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:hotkey_manager/hotkey_manager.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'window_manager_provider.dart';

part 'hotkey_manager_provider.g.dart';

/// 全局快捷键管理器，负责注册系统级快捷键以快速呼出剪贴板历史窗口。
@Riverpod(keepAlive: true)
class AppHotKeyManager extends _$AppHotKeyManager {
  @override
  FutureOr<void> build() async {
    debugPrint('🚀 AppHotKeyManager: 开始初始化热键...');
    final configStr = ref.watch(hotkeyOpenProvider);
    await _setupHotkey(configStr);

    ref.onDispose(() {
      debugPrint('🛑 AppHotKeyManager: 正在注销所有热键...');
      hotKeyManager.unregisterAll();
    });
    debugPrint('✅ AppHotKeyManager: 热键服务就绪');
  }

  /// 设置并注册快捷键，会先注销所有现有快捷键
  Future<void> _setupHotkey(String configStr) async {
    await hotKeyManager.unregisterAll();
    
    final config = AppHotKeyConfig.fromRawJson(configStr);
    final hotKey = config.toHotKey();

    if (hotKey != null) {
      try {
        await hotKeyManager.register(
          hotKey,
          keyDownHandler: (_) {
            debugPrint('⌨️ 热键触发: 切换窗口显示状态');
            ref.read(appWindowManagerProvider.notifier).toggleHistory(source: TriggerSource.hotkey);
          },
        );
        debugPrint('✅ 热键注册成功: ${hotKey.toJson()}');
      } catch (e) {
        debugPrint('❌ 热键注册失败: $e');
      }
    } else {
      debugPrint('⚠️ 未配置有效热键');
    }
  }
}
