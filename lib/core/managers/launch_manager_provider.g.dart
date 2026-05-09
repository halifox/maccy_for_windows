// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'launch_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 开机自启管理器。
///
/// 负责管理应用程序在操作系统登录时的自动启动配置。
/// 依赖于 launch_at_startup 包来实现跨平台支持。

@ProviderFor(AppLaunchManager)
final appLaunchManagerProvider = AppLaunchManagerProvider._();

/// 开机自启管理器。
///
/// 负责管理应用程序在操作系统登录时的自动启动配置。
/// 依赖于 launch_at_startup 包来实现跨平台支持。
final class AppLaunchManagerProvider
    extends $AsyncNotifierProvider<AppLaunchManager, void> {
  /// 开机自启管理器。
  ///
  /// 负责管理应用程序在操作系统登录时的自动启动配置。
  /// 依赖于 launch_at_startup 包来实现跨平台支持。
  AppLaunchManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appLaunchManagerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appLaunchManagerHash();

  @$internal
  @override
  AppLaunchManager create() => AppLaunchManager();
}

String _$appLaunchManagerHash() => r'af57c5f57a1eb28d7bfe05a7a457e6117caf2cb6';

/// 开机自启管理器。
///
/// 负责管理应用程序在操作系统登录时的自动启动配置。
/// 依赖于 launch_at_startup 包来实现跨平台支持。

abstract class _$AppLaunchManager extends $AsyncNotifier<void> {
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
