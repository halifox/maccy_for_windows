// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'history_providers.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 历史记录搜索查询 Provider

@ProviderFor(HistorySearchQuery)
final historySearchQueryProvider = HistorySearchQueryProvider._();

/// 历史记录搜索查询 Provider
final class HistorySearchQueryProvider
    extends $NotifierProvider<HistorySearchQuery, String> {
  /// 历史记录搜索查询 Provider
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

/// 历史记录搜索查询 Provider

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

/// 历史记录当前选中索引 Provider

@ProviderFor(HistorySelectedIndex)
final historySelectedIndexProvider = HistorySelectedIndexProvider._();

/// 历史记录当前选中索引 Provider
final class HistorySelectedIndexProvider
    extends $NotifierProvider<HistorySelectedIndex, int> {
  /// 历史记录当前选中索引 Provider
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

/// 历史记录当前选中索引 Provider

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

/// 历史记录焦点请求 Provider，用于触发搜索框获取焦点

@ProviderFor(HistoryFocusRequest)
final historyFocusRequestProvider = HistoryFocusRequestProvider._();

/// 历史记录焦点请求 Provider，用于触发搜索框获取焦点
final class HistoryFocusRequestProvider
    extends $NotifierProvider<HistoryFocusRequest, int> {
  /// 历史记录焦点请求 Provider，用于触发搜索框获取焦点
  HistoryFocusRequestProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'historyFocusRequestProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$historyFocusRequestHash();

  @$internal
  @override
  HistoryFocusRequest create() => HistoryFocusRequest();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(int value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<int>(value),
    );
  }
}

String _$historyFocusRequestHash() =>
    r'5abbaf064177c6a49fb0c42d249e5ffd686814cc';

/// 历史记录焦点请求 Provider，用于触发搜索框获取焦点

abstract class _$HistoryFocusRequest extends $Notifier<int> {
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

/// 经过过滤和排序的历史记录流 Provider

@ProviderFor(filteredHistory)
final filteredHistoryProvider = FilteredHistoryProvider._();

/// 经过过滤和排序的历史记录流 Provider

final class FilteredHistoryProvider
    extends
        $FunctionalProvider<
          AsyncValue<List<ClipboardEntry>>,
          List<ClipboardEntry>,
          Stream<List<ClipboardEntry>>
        >
    with
        $FutureModifier<List<ClipboardEntry>>,
        $StreamProvider<List<ClipboardEntry>> {
  /// 经过过滤和排序的历史记录流 Provider
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
  $StreamProviderElement<List<ClipboardEntry>> $createElement(
    $ProviderPointer pointer,
  ) => $StreamProviderElement(pointer);

  @override
  Stream<List<ClipboardEntry>> create(Ref ref) {
    return filteredHistory(ref);
  }
}

String _$filteredHistoryHash() => r'594d3b3b8c1f7979b2e8421b57b1c789d825522a';

/// 历史记录控制器，负责处理用户交互操作（选择、删除、置顶、清空等）

@ProviderFor(HistoryController)
final historyControllerProvider = HistoryControllerProvider._();

/// 历史记录控制器，负责处理用户交互操作（选择、删除、置顶、清空等）
final class HistoryControllerProvider
    extends $NotifierProvider<HistoryController, void> {
  /// 历史记录控制器，负责处理用户交互操作（选择、删除、置顶、清空等）
  HistoryControllerProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'historyControllerProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$historyControllerHash();

  @$internal
  @override
  HistoryController create() => HistoryController();

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(void value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<void>(value),
    );
  }
}

String _$historyControllerHash() => r'c8c1f1314c367e4d2ddbef49f62c2911a81b5fc4';

/// 历史记录控制器，负责处理用户交互操作（选择、删除、置顶、清空等）

abstract class _$HistoryController extends $Notifier<void> {
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
