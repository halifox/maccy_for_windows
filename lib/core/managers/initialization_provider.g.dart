// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'initialization_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(appStartup)
final appStartupProvider = AppStartupProvider._();

final class AppStartupProvider
    extends $FunctionalProvider<AsyncValue<void>, void, FutureOr<void>>
    with $FutureModifier<void>, $FutureProvider<void> {
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

String _$appStartupHash() => r'2ceaf08b1c50a26d9d46e8b1957c5600ccebf8aa';
