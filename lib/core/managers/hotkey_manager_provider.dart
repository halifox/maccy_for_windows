import 'package:maccy/core/models/hotkey_config.dart';
import 'package:maccy/core/services/modifier_key_service.dart';
import 'package:maccy/features/history/providers/history_providers.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';
import 'package:flutter/cupertino.dart';
import 'package:hotkey_manager/hotkey_manager.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'package:maccy/core/managers/window_manager_provider.dart';

part 'hotkey_manager_provider.g.dart';

/// 弹窗状态枚举。
///
/// 对应 Maccy 的 PopupState，实现三态状态机。
enum PopupState {
  /// 默认模式：按一次打开，再按关闭。
  toggle,

  /// 循环模式：按住修饰键连续按主键循环选择。
  cycle,

  /// 过渡状态：刚按下快捷键，尚未确定是 toggle 还是 cycle。
  opening,
}

/// 全局热键管理器。
///
/// 负责在系统底层注册快捷键，以便在任何应用程序中都能通过特定按键组合
/// 快速唤起或切换 Maccy 的剪贴板历史主窗口。
/// 实现 Maccy 的循环选择模式（Cycle Mode）。
@Riverpod(keepAlive: true)
class AppHotKeyManager extends _$AppHotKeyManager {
  PopupState _popupState = PopupState.toggle;
  final ModifierKeyService _modifierKeyService = ModifierKeyService();

  /// 初始化热键管理器。
  ///
  /// 监听设置中的快捷键配置变化，并实时更新系统级热键注册。
  @override
  FutureOr<void> build() async {
    await _setupHotkey();
    ref.listen(hotkeyOpenProvider, (previous, next) {
      _setupHotkey();
    });

    ref.onDispose(() {
      hotKeyManager.unregisterAll();
      _modifierKeyService.dispose();
    });
  }

  /// 设置并注册系统热键。
  ///
  /// 首先注销已有的所有热键以防冲突，然后根据配置注册。
  /// 仅当配置中包含至少一个修饰键时才进行注册，避免误触。
  Future<void> _setupHotkey() async {
    await hotKeyManager.unregisterAll();
    final AppHotKeyConfig config = ref.read(hotkeyOpenProvider);
    // 验证：必须包含至少一个修饰键（Ctrl, Alt, Shift, Meta），不能单独使用主键位
    if (config.modifiers.isEmpty) {
      return;
    }

    final hotKey = config.toHotKey();

    if (hotKey != null) {
      try {
        await hotKeyManager.register(
          hotKey,
          keyDownHandler: (_) => _handleHotkeyPressed(),
        );
      } catch (e) {
        debugPrint(e.toString());
      }
    }
  }

  /// 处理热键按下事件。
  ///
  /// 实现 Maccy 的三态状态机逻辑：
  /// - toggle: 按一次打开，再按关闭
  /// - opening: 刚打开窗口，等待判断是否进入 cycle 模式
  /// - cycle: 按住修饰键连续按主键循环选择
  void _handleHotkeyPressed() {
    final windowManager = ref.read(appWindowManagerProvider.notifier);
    final isVisible = windowManager.isShowing;

    if (!isVisible) {
      // 窗口未显示，打开窗口并进入 opening 状态
      windowManager.showHistory(source: TriggerSource.hotkey);
      _popupState = PopupState.opening;

      // 200ms 后如果未再次按键，自动进入 toggle 模式
      Future.delayed(const Duration(milliseconds: 200), () {
        if (_popupState == PopupState.opening) {
          _popupState = PopupState.toggle;
        }
      });

      // 开始监听修饰键释放
      _startModifierMonitoring();
    } else {
      // 窗口已显示
      if (_popupState == PopupState.opening) {
        // 从 opening 进入 cycle 模式
        _popupState = PopupState.cycle;
      }

      if (_popupState == PopupState.cycle) {
        // 循环模式：选择下一项
        ref.read(historyControllerProvider.notifier).selectNext(cycle: true);
      } else if (_popupState == PopupState.toggle) {
        // Toggle 模式：关闭窗口
        windowManager.hideHistory();
        _resetState();
      }
    }
  }

  /// 开始监听修饰键释放。
  void _startModifierMonitoring() {
    _modifierKeyService.startMonitoring(
      onAllModifiersReleased: _handleModifiersReleased,
    );
  }

  /// 处理修饰键释放事件。
  ///
  /// 在 cycle 模式下，释放修饰键时自动粘贴选中项并关闭窗口。
  void _handleModifiersReleased() {
    if (_popupState == PopupState.cycle) {
      // 粘贴选中项
      final id = ref.read(historySelectedIdProvider);
      if(id!=null){
        ref.read(historyControllerProvider.notifier).selectItem(id);
      }

      // 关闭窗口
      ref.read(appWindowManagerProvider.notifier).hideHistory();

      _resetState();
    }
  }

  /// 重置状态。
  void _resetState() {
    _popupState = PopupState.toggle;
    _modifierKeyService.stopMonitoring();
  }

  /// 获取当前弹窗状态（用于调试）。
  PopupState get currentState => _popupState;
}
