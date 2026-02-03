// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'hotkey_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 全局快捷键管理器，负责注册系统级快捷键以快速呼出剪贴板历史窗口。

@ProviderFor(AppHotKeyManager)
final appHotKeyManagerProvider = AppHotKeyManagerProvider._();

/// 全局快捷键管理器，负责注册系统级快捷键以快速呼出剪贴板历史窗口。
final class AppHotKeyManagerProvider
    extends $AsyncNotifierProvider<AppHotKeyManager, void> {
  /// 全局快捷键管理器，负责注册系统级快捷键以快速呼出剪贴板历史窗口。
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

String _$appHotKeyManagerHash() => r'd3183b274f56000b6f901228e6e4bac98fbc0da9';

/// 全局快捷键管理器，负责注册系统级快捷键以快速呼出剪贴板历史窗口。

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
