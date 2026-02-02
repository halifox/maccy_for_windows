// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'history_providers.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning

@ProviderFor(HistorySearchQuery)
final historySearchQueryProvider = HistorySearchQueryProvider._();

final class HistorySearchQueryProvider
    extends $NotifierProvider<HistorySearchQuery, String> {
  HistorySearchQueryProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'historySearchQueryProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$historySearchQueryHash();

  @$internal
  @override
  HistorySearchQuery create() => HistorySearchQuery();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(String value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<String>(value),
    );
  }
}

String _$historySearchQueryHash() =>
    r'89f46053fb648ac09a67c8521aed87dfa229f048';

abstract class _$HistorySearchQuery extends $Notifier<String> {
  String build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<String, String>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<String, String>,
              String,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}

@ProviderFor(HistorySelectedIndex)
final historySelectedIndexProvider = HistorySelectedIndexProvider._();

final class HistorySelectedIndexProvider
    extends $NotifierProvider<HistorySelectedIndex, int> {
  HistorySelectedIndexProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'historySelectedIndexProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$historySelectedIndexHash();

  @$internal
  @override
  HistorySelectedIndex create() => HistorySelectedIndex();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(int value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<int>(value),
    );
  }
}

String _$historySelectedIndexHash() =>
    r'425ec12dcebe77a16ed3d53e939050f322188e4b';

abstract class _$HistorySelectedIndex extends $Notifier<int> {
  int build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<int, int>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<int, int>,
              int,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}

@ProviderFor(historyEntries)
final historyEntriesProvider = HistoryEntriesProvider._();

final class HistoryEntriesProvider
    extends
        $FunctionalProvider<
          AsyncValue<List<ClipboardEntry>>,
          List<ClipboardEntry>,
          Stream<List<ClipboardEntry>>
        >
    with
        $FutureModifier<List<ClipboardEntry>>,
        $StreamProvider<List<ClipboardEntry>> {
  HistoryEntriesProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'historyEntriesProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$historyEntriesHash();

  @$internal
  @override
  $StreamProviderElement<List<ClipboardEntry>> $createElement(
    $ProviderPointer pointer,
  ) => $StreamProviderElement(pointer);

  @override
  Stream<List<ClipboardEntry>> create(Ref ref) {
    return historyEntries(ref);
  }
}

String _$historyEntriesHash() => r'26e1b16528f1fd4301815bfe10ce7848229a1005';

@ProviderFor(filteredPins)
final filteredPinsProvider = FilteredPinsProvider._();

final class FilteredPinsProvider
    extends
        $FunctionalProvider<
          List<ClipboardEntry>,
          List<ClipboardEntry>,
          List<ClipboardEntry>
        >
    with $Provider<List<ClipboardEntry>> {
  FilteredPinsProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'filteredPinsProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$filteredPinsHash();

  @$internal
  @override
  $ProviderElement<List<ClipboardEntry>> $createElement(
    $ProviderPointer pointer,
  ) => $ProviderElement(pointer);

  @override
  List<ClipboardEntry> create(Ref ref) {
    return filteredPins(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(List<ClipboardEntry> value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<List<ClipboardEntry>>(value),
    );
  }
}

String _$filteredPinsHash() => r'36300a32af240f189af7ea862e2f8f2757ad92a5';

@ProviderFor(filteredHistory)
final filteredHistoryProvider = FilteredHistoryProvider._();

final class FilteredHistoryProvider
    extends
        $FunctionalProvider<
          List<ClipboardEntry>,
          List<ClipboardEntry>,
          List<ClipboardEntry>
        >
    with $Provider<List<ClipboardEntry>> {
  FilteredHistoryProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'filteredHistoryProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$filteredHistoryHash();

  @$internal
  @override
  $ProviderElement<List<ClipboardEntry>> $createElement(
    $ProviderPointer pointer,
  ) => $ProviderElement(pointer);

  @override
  List<ClipboardEntry> create(Ref ref) {
    return filteredHistory(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(List<ClipboardEntry> value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<List<ClipboardEntry>>(value),
    );
  }
}

String _$filteredHistoryHash() => r'fd5586b4cfc933c23c324d75ad796e3228e57a15';

@ProviderFor(HistoryActions)
final historyActionsProvider = HistoryActionsProvider._();

final class HistoryActionsProvider
    extends $NotifierProvider<HistoryActions, void> {
  HistoryActionsProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'historyActionsProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$historyActionsHash();

  @$internal
  @override
  HistoryActions create() => HistoryActions();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(void value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<void>(value),
    );
  }
}

String _$historyActionsHash() => r'25039df90f324d8c4ae196faaaa5bca852cc73e5';

abstract class _$HistoryActions extends $Notifier<void> {
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
