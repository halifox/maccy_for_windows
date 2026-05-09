// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'popup_state_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// Popup 状态管理器。
///
/// 实现 Maccy 的弹窗状态逻辑：
/// 1. 首次按下快捷键 → opening 状态
/// 2. 如果在 opening 状态下再次按下主键 → 进入 cycle 模式
/// 3. 如果在 opening 状态下释放修饰键 → 进入 toggle 模式
/// 4. 在 cycle 模式下释放修饰键 → 自动粘贴选中项并关闭

@ProviderFor(PopupStateManager)
final popupStateManagerProvider = PopupStateManagerProvider._();

/// Popup 状态管理器。
///
/// 实现 Maccy 的弹窗状态逻辑：
/// 1. 首次按下快捷键 → opening 状态
/// 2. 如果在 opening 状态下再次按下主键 → 进入 cycle 模式
/// 3. 如果在 opening 状态下释放修饰键 → 进入 toggle 模式
/// 4. 在 cycle 模式下释放修饰键 → 自动粘贴选中项并关闭
final class PopupStateManagerProvider
    extends $NotifierProvider<PopupStateManager, PopupState> {
  /// Popup 状态管理器。
  ///
  /// 实现 Maccy 的弹窗状态逻辑：
  /// 1. 首次按下快捷键 → opening 状态
  /// 2. 如果在 opening 状态下再次按下主键 → 进入 cycle 模式
  /// 3. 如果在 opening 状态下释放修饰键 → 进入 toggle 模式
  /// 4. 在 cycle 模式下释放修饰键 → 自动粘贴选中项并关闭
  PopupStateManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'popupStateManagerProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$popupStateManagerHash();

  @$internal
  @override
  PopupStateManager create() => PopupStateManager();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(PopupState value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<PopupState>(value),
    );
  }
}

String _$popupStateManagerHash() => r'cb184ace33514e403a328178b4640e6557630d19';

/// Popup 状态管理器。
///
/// 实现 Maccy 的弹窗状态逻辑：
/// 1. 首次按下快捷键 → opening 状态
/// 2. 如果在 opening 状态下再次按下主键 → 进入 cycle 模式
/// 3. 如果在 opening 状态下释放修饰键 → 进入 toggle 模式
/// 4. 在 cycle 模式下释放修饰键 → 自动粘贴选中项并关闭

abstract class _$PopupStateManager extends $Notifier<PopupState> {
  PopupState build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<PopupState, PopupState>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<PopupState, PopupState>,
              PopupState,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}

/// 修饰键状态管理器。
///
/// 监听并跟踪修饰键（Alt/Ctrl/Shift/Meta）的按下状态。

@ProviderFor(ModifierKeysState)
final modifierKeysStateProvider = ModifierKeysStateProvider._();

/// 修饰键状态管理器。
///
/// 监听并跟踪修饰键（Alt/Ctrl/Shift/Meta）的按下状态。
final class ModifierKeysStateProvider
    extends $NotifierProvider<ModifierKeysState, Set<LogicalKeyboardKey>> {
  /// 修饰键状态管理器。
  ///
  /// 监听并跟踪修饰键（Alt/Ctrl/Shift/Meta）的按下状态。
  ModifierKeysStateProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'modifierKeysStateProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$modifierKeysStateHash();

  @$internal
  @override
  ModifierKeysState create() => ModifierKeysState();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(Set<LogicalKeyboardKey> value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<Set<LogicalKeyboardKey>>(value),
    );
  }
}

String _$modifierKeysStateHash() => r'c373f6eee5cd8b929b0585c96ef1bbf7a3e88f7c';

/// 修饰键状态管理器。
///
/// 监听并跟踪修饰键（Alt/Ctrl/Shift/Meta）的按下状态。

abstract class _$ModifierKeysState extends $Notifier<Set<LogicalKeyboardKey>> {
  Set<LogicalKeyboardKey> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref =
        this.ref as $Ref<Set<LogicalKeyboardKey>, Set<LogicalKeyboardKey>>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<Set<LogicalKeyboardKey>, Set<LogicalKeyboardKey>>,
              Set<LogicalKeyboardKey>,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}
