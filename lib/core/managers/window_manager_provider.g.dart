// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'window_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 窗口管理器，负责主窗口（剪贴板历史、设置界面）的显示、隐藏、定位以及生命周期管理。

@ProviderFor(AppWindowManager)
final appWindowManagerProvider = AppWindowManagerProvider._();

/// 窗口管理器，负责主窗口（剪贴板历史、设置界面）的显示、隐藏、定位以及生命周期管理。
final class AppWindowManagerProvider
    extends $AsyncNotifierProvider<AppWindowManager, void> {
  /// 窗口管理器，负责主窗口（剪贴板历史、设置界面）的显示、隐藏、定位以及生命周期管理。
  AppWindowManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appWindowManagerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appWindowManagerHash();

  @$internal
  @override
  AppWindowManager create() => AppWindowManager();
}

String _$appWindowManagerHash() => r'73360cc4fa728976d520964eca1d31798c2495a6';

/// 窗口管理器，负责主窗口（剪贴板历史、设置界面）的显示、隐藏、定位以及生命周期管理。

abstract class _$AppWindowManager extends $AsyncNotifier<void> {
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
