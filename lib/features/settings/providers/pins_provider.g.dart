// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'pins_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(PinsNotifier)
final pinsProvider = PinsNotifierProvider._();

final class PinsNotifierProvider
    extends $StreamNotifierProvider<PinsNotifier, List<Pin>> {
  PinsNotifierProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'pinsProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$pinsNotifierHash();

  @$internal
  @override
  PinsNotifier create() => PinsNotifier();
}

String _$pinsNotifierHash() => r'4ec212387eb8345f4b31c00d847b6c4dcd9754d5';

abstract class _$PinsNotifier extends $StreamNotifier<List<Pin>> {
  Stream<List<Pin>> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<AsyncValue<List<Pin>>, List<Pin>>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<AsyncValue<List<Pin>>, List<Pin>>,
              AsyncValue<List<Pin>>,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}
