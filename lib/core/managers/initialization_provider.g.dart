// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'initialization_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 应用程序启动初始化 Provider，用于预先加载并运行所有的后台管理服务（窗口、托盘、快捷键、剪贴板监听）。

@ProviderFor(appStartup)
final appStartupProvider = AppStartupProvider._();

/// 应用程序启动初始化 Provider，用于预先加载并运行所有的后台管理服务（窗口、托盘、快捷键、剪贴板监听）。

final class AppStartupProvider
    extends $FunctionalProvider<AsyncValue<void>, void, FutureOr<void>>
    with $FutureModifier<void>, $FutureProvider<void> {
  /// 应用程序启动初始化 Provider，用于预先加载并运行所有的后台管理服务（窗口、托盘、快捷键、剪贴板监听）。
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

String _$appStartupHash() => r'a0db7950e9eb389f12320c4a4a02f13fe3899c53';
