// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'initialization_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 应用程序启动初始化流程编排器。
///
/// 作为应用启动的“哨兵”，负责协调所有核心后台管理服务的并行初始化过程。
/// 只有当本 Provider 完成后，主 UI 才会从闪屏页切换到主业务页面。
///
/// [ref] Riverpod 引用，用于触发各管理器的实例化。

@ProviderFor(appStartup)
final appStartupProvider = AppStartupProvider._();

/// 应用程序启动初始化流程编排器。
///
/// 作为应用启动的“哨兵”，负责协调所有核心后台管理服务的并行初始化过程。
/// 只有当本 Provider 完成后，主 UI 才会从闪屏页切换到主业务页面。
///
/// [ref] Riverpod 引用，用于触发各管理器的实例化。

final class AppStartupProvider
    extends $FunctionalProvider<AsyncValue<void>, void, FutureOr<void>>
    with $FutureModifier<void>, $FutureProvider<void> {
  /// 应用程序启动初始化流程编排器。
  ///
  /// 作为应用启动的“哨兵”，负责协调所有核心后台管理服务的并行初始化过程。
  /// 只有当本 Provider 完成后，主 UI 才会从闪屏页切换到主业务页面。
  ///
  /// [ref] Riverpod 引用，用于触发各管理器的实例化。
  AppStartupProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appStartupProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appStartupHash();

  @$internal
  @override
  $FutureProviderElement<void> $createElement($ProviderPointer pointer) =>
      $FutureProviderElement(pointer);

  @override
  FutureOr<void> create(Ref ref) {
    return appStartup(ref);
  }
}

String _$appStartupHash() => r'545917f6fd7f7eb8777e9ed98bbb2cd321d39659';
