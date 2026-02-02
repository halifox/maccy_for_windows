// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'clipboard_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(AppClipboardManager)
final appClipboardManagerProvider = AppClipboardManagerProvider._();

final class AppClipboardManagerProvider
    extends $NotifierProvider<AppClipboardManager, void> {
  AppClipboardManagerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appClipboardManagerProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appClipboardManagerHash();

  @$internal
  @override
  AppClipboardManager create() => AppClipboardManager();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(void value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<void>(value),
    );
  }
}

String _$appClipboardManagerHash() =>
    r'934a66f7f0c8fdeeb0bc14497bfd8a3b53fc9877';

abstract class _$AppClipboardManager extends $Notifier<void> {
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
