// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'tray_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 系统托盘管理器，负责初始化托盘图标、设置悬停提示以及监听托盘点击事件。

@ProviderFor(AppTrayManager)
final appTrayManagerProvider = AppTrayManagerProvider._();

/// 系统托盘管理器，负责初始化托盘图标、设置悬停提示以及监听托盘点击事件。
final class AppTrayManagerProvider
    extends $AsyncNotifierProvider<AppTrayManager, void> {
  /// 系统托盘管理器，负责初始化托盘图标、设置悬停提示以及监听托盘点击事件。
  AppTrayManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appTrayManagerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appTrayManagerHash();

  @$internal
  @override
  AppTrayManager create() => AppTrayManager();
}

String _$appTrayManagerHash() => r'c2ce56c81e349de770e7ee1d15aea209b5228d52';

/// 系统托盘管理器，负责初始化托盘图标、设置悬停提示以及监听托盘点击事件。

abstract class _$AppTrayManager extends $AsyncNotifier<void> {
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
