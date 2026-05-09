// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'hotkey_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 全局热键管理器。
///
/// 负责在系统底层注册快捷键，以便在任何应用程序中都能通过特定按键组合
/// 快速唤起或切换 Maccy 的剪贴板历史主窗口。
/// 实现 Maccy 的循环选择模式（Cycle Mode）。

@ProviderFor(AppHotKeyManager)
final appHotKeyManagerProvider = AppHotKeyManagerProvider._();

/// 全局热键管理器。
///
/// 负责在系统底层注册快捷键，以便在任何应用程序中都能通过特定按键组合
/// 快速唤起或切换 Maccy 的剪贴板历史主窗口。
/// 实现 Maccy 的循环选择模式（Cycle Mode）。
final class AppHotKeyManagerProvider
    extends $AsyncNotifierProvider<AppHotKeyManager, void> {
  /// 全局热键管理器。
  ///
  /// 负责在系统底层注册快捷键，以便在任何应用程序中都能通过特定按键组合
  /// 快速唤起或切换 Maccy 的剪贴板历史主窗口。
  /// 实现 Maccy 的循环选择模式（Cycle Mode）。
  AppHotKeyManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appHotKeyManagerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appHotKeyManagerHash();

  @$internal
  @override
  AppHotKeyManager create() => AppHotKeyManager();
}

String _$appHotKeyManagerHash() => r'0742827763d27c691dd3ad5eb38f79cba191fab3';

/// 全局热键管理器。
///
/// 负责在系统底层注册快捷键，以便在任何应用程序中都能通过特定按键组合
/// 快速唤起或切换 Maccy 的剪贴板历史主窗口。
/// 实现 Maccy 的循环选择模式（Cycle Mode）。

abstract class _$AppHotKeyManager extends $AsyncNotifier<void> {
  FutureOr<void> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<AsyncValue<void>, void>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<AsyncValue<void>, void>,
              AsyncValue<void>,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}
