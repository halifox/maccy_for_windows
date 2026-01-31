// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'window_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(AppWindowManager)
final appWindowManagerProvider = AppWindowManagerProvider._();

final class AppWindowManagerProvider
    extends $NotifierProvider<AppWindowManager, void> {
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

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(void value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<void>(value),
    );
  }
}

String _$appWindowManagerHash() => r'5932139676fa3c207822d325d7211e33107616f1';

abstract class _$AppWindowManager extends $Notifier<void> {
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
