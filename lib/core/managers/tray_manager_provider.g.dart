// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'tray_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(AppTrayManager)
final appTrayManagerProvider = AppTrayManagerProvider._();

final class AppTrayManagerProvider
    extends $NotifierProvider<AppTrayManager, void> {
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

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(void value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<void>(value),
    );
  }
}

String _$appTrayManagerHash() => r'f6eab252d52179203d861ab15443d93169d7754c';

abstract class _$AppTrayManager extends $Notifier<void> {
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
