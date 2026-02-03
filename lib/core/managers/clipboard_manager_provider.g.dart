// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'clipboard_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 剪贴板管理器，负责监听系统剪贴板变化、处理数据过滤并持久化到数据库。

@ProviderFor(AppClipboardManager)
final appClipboardManagerProvider = AppClipboardManagerProvider._();

/// 剪贴板管理器，负责监听系统剪贴板变化、处理数据过滤并持久化到数据库。
final class AppClipboardManagerProvider
    extends $AsyncNotifierProvider<AppClipboardManager, void> {
  /// 剪贴板管理器，负责监听系统剪贴板变化、处理数据过滤并持久化到数据库。
  AppClipboardManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appClipboardManagerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appClipboardManagerHash();

  @$internal
  @override
  AppClipboardManager create() => AppClipboardManager();
}

String _$appClipboardManagerHash() =>
    r'21a29476f4ba8c1cf8f51d234b2a594695f87476';

/// 剪贴板管理器，负责监听系统剪贴板变化、处理数据过滤并持久化到数据库。

abstract class _$AppClipboardManager extends $AsyncNotifier<void> {
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
