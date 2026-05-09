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
/// 字段说明:
/// [_cleanupTimer] 定时清理任务的计时器，用于定期执行数据库容量缩减。

@ProviderFor(AppClipboardManager)
final appClipboardManagerProvider = AppClipboardManagerProvider._();

/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 字段说明:
/// [_cleanupTimer] 定时清理任务的计时器，用于定期执行数据库容量缩减。
final class AppClipboardManagerProvider
    extends $AsyncNotifierProvider<AppClipboardManager, void> {
  /// 剪贴板管理器。
  ///
  /// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
  /// 以及跨平台的自动粘贴模拟操作。
  ///
  /// 字段说明:
  /// [_cleanupTimer] 定时清理任务的计时器，用于定期执行数据库容量缩减。
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
    r'd9a62fc6de0743a1480c65f2480af420edc5cda5';

/// 剪贴板管理器。
///
/// 负责监听系统剪贴板变化、执行数据去重与过滤、持久化条目到数据库、
/// 以及跨平台的自动粘贴模拟操作。
///
/// 字段说明:
/// [_cleanupTimer] 定时清理任务的计时器，用于定期执行数据库容量缩减。

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
