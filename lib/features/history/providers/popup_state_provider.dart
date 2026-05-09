import 'package:flutter/services.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

part 'popup_state_provider.g.dart';

/// Popup 状态枚举（对应 Maccy 的 PopupState）。
///
/// - toggle: 默认模式，快捷键切换弹窗的打开/关闭
/// - cycle: 循环模式，按住修饰键连续按主键时循环选择下一项
/// - opening: 过渡状态，快捷键首次按下时的状态，用于判断进入 toggle 还是 cycle 模式
enum PopupState {
  toggle,
  cycle,
  opening,
}

/// Popup 状态管理器。
///
/// 实现 Maccy 的弹窗状态逻辑：
/// 1. 首次按下快捷键 → opening 状态
/// 2. 如果在 opening 状态下再次按下主键 → 进入 cycle 模式
/// 3. 如果在 opening 状态下释放修饰键 → 进入 toggle 模式
/// 4. 在 cycle 模式下释放修饰键 → 自动粘贴选中项并关闭
@riverpod
class PopupStateManager extends _$PopupStateManager {
  @override
  PopupState build() => PopupState.toggle;

  /// 重置为 toggle 模式。
  void reset() {
    state = PopupState.toggle;
  }

  /// 处理快捷键按下事件。
  ///
  /// [isMainKey] 是否为主键（非修饰键）
  /// [isClosed] 弹窗是否关闭
  ///
  /// 返回是否应该打开弹窗。
  bool handleKeyDown(bool isMainKey, bool isClosed) {
    if (isClosed) {
      // 弹窗关闭时，按下快捷键进入 opening 状态
      state = PopupState.opening;
      return true; // 打开弹窗
    }

    // 弹窗已打开
    if (isMainKey) {
      if (state == PopupState.opening) {
        // 在 opening 状态下再次按主键 → 进入 cycle 模式
        state = PopupState.cycle;
        return false; // 不需要重新打开
      }

      if (state == PopupState.cycle) {
        // 在 cycle 模式下按主键 → 循环到下一项
        return false; // 由调用方处理循环逻辑
      }

      if (state == PopupState.toggle) {
        // 在 toggle 模式下按主键 → 关闭弹窗
        return false; // 由调用方处理关闭逻辑
      }
    }

    return false;
  }

  /// 处理修饰键释放事件。
  ///
  /// [allModifiersReleased] 是否所有修饰键都已释放
  ///
  /// 返回是否应该自动粘贴并关闭。
  bool handleFlagsChanged(bool allModifiersReleased) {
    if (allModifiersReleased) {
      if (state == PopupState.cycle) {
        // 在 cycle 模式下释放修饰键 → 自动粘贴并关闭
        reset();
        return true;
      }

      if (state == PopupState.opening) {
        // 在 opening 状态下释放修饰键 → 进入 toggle 模式
        state = PopupState.toggle;
        return false;
      }
    }

    return false;
  }

  /// 获取当前状态。
  PopupState get currentState => state;

  /// 是否处于 cycle 模式。
  bool get isCycleMode => state == PopupState.cycle;

  /// 是否处于 opening 状态。
  bool get isOpening => state == PopupState.opening;

  /// 是否处于 toggle 模式。
  bool get isToggleMode => state == PopupState.toggle;
}

/// 修饰键状态管理器。
///
/// 监听并跟踪修饰键（Alt/Ctrl/Shift/Meta）的按下状态。
@riverpod
class ModifierKeysState extends _$ModifierKeysState {
  @override
  Set<LogicalKeyboardKey> build() => {};

  /// 更新修饰键状态。
  void update(RawKeyEvent event) {
    final modifiers = <LogicalKeyboardKey>{};

    if (HardwareKeyboard.instance.isAltPressed) {
      modifiers.add(LogicalKeyboardKey.alt);
    }
    if (HardwareKeyboard.instance.isControlPressed) {
      modifiers.add(LogicalKeyboardKey.control);
    }
    if (HardwareKeyboard.instance.isShiftPressed) {
      modifiers.add(LogicalKeyboardKey.shift);
    }
    if (HardwareKeyboard.instance.isMetaPressed) {
      modifiers.add(LogicalKeyboardKey.meta);
    }

    state = modifiers;
  }

  /// 是否所有修饰键都已释放。
  bool get allReleased => state.isEmpty;

  /// 是否按下了 Alt 键。
  bool get isAltPressed => state.contains(LogicalKeyboardKey.alt);

  /// 是否按下了 Ctrl 键。
  bool get isControlPressed => state.contains(LogicalKeyboardKey.control);

  /// 是否按下了 Shift 键。
  bool get isShiftPressed => state.contains(LogicalKeyboardKey.shift);

  /// 是否按下了 Meta 键（Windows 键 / Command 键）。
  bool get isMetaPressed => state.contains(LogicalKeyboardKey.meta);
}
