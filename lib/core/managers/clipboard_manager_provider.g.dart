// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'clipboard_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 实现 Maccy 的多格式内容存储逻辑。

@ProviderFor(AppClipboardManager)
final appClipboardManagerProvider = AppClipboardManagerProvider._();

/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 实现 Maccy 的多格式内容存储逻辑。
final class AppClipboardManagerProvider
    extends $AsyncNotifierProvider<AppClipboardManager, void> {
  /// 剪贴板管理器。
  ///
  /// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
  /// 以及跨平台的自动粘贴模拟操作。
  ///
  /// 实现 Maccy 的多格式内容存储逻辑。
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
    r'5d173ab6670030b3c32e29f432c5fdbe42b12947';

/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 实现 Maccy 的多格式内容存储逻辑。

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
