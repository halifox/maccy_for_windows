// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'hotkey_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(AppHotKeyManager)
final appHotKeyManagerProvider = AppHotKeyManagerProvider._();

final class AppHotKeyManagerProvider
    extends $NotifierProvider<AppHotKeyManager, void> {
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

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(void value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<void>(value),
    );
  }
}

String _$appHotKeyManagerHash() => r'15ac13f529ac56181325f43293ccdbe715d03a26';

abstract class _$AppHotKeyManager extends $Notifier<void> {
  void build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<void, void>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<void, void>,
              void,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}
