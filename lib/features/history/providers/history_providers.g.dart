// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'history_providers.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 历史记录搜索查询 Notifier。
///
/// 管理搜索框中的实时输入内容。

@ProviderFor(HistorySearchQuery)
final historySearchQueryProvider = HistorySearchQueryProvider._();

/// 历史记录搜索查询 Notifier。
///
/// 管理搜索框中的实时输入内容。
final class HistorySearchQueryProvider
    extends $NotifierProvider<HistorySearchQuery, String> {
  /// 历史记录搜索查询 Notifier。
  ///
  /// 管理搜索框中的实时输入内容。
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
    r'bcabdde7ad84ad21dc8901ef0b2871dfb499de6b';

/// 历史记录搜索查询 Notifier。
///
/// 管理搜索框中的实时输入内容。

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

/// 历史记录选中项索引 Notifier。
///
/// 管理列表中的焦点位置（键盘上下键导航的当前项）。

@ProviderFor(HistorySelectedIndex)
final historySelectedIndexProvider = HistorySelectedIndexProvider._();

/// 历史记录选中项索引 Notifier。
///
/// 管理列表中的焦点位置（键盘上下键导航的当前项）。
final class HistorySelectedIndexProvider
    extends $NotifierProvider<HistorySelectedIndex, int> {
  /// 历史记录选中项索引 Notifier。
  ///
  /// 管理列表中的焦点位置（键盘上下键导航的当前项）。
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
    r'97c194c35ad7c655e8f3720aee9ef352c43b5a47';

/// 历史记录选中项索引 Notifier。
///
/// 管理列表中的焦点位置（键盘上下键导航的当前项）。

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

/// 历史记录焦点请求 Notifier。
///
/// 用于在窗口弹出时强制触发 UI 层的文本框 Focus 操作。

@ProviderFor(HistoryFocusRequest)
final historyFocusRequestProvider = HistoryFocusRequestProvider._();

/// 历史记录焦点请求 Notifier。
///
/// 用于在窗口弹出时强制触发 UI 层的文本框 Focus 操作。
final class HistoryFocusRequestProvider
    extends $NotifierProvider<HistoryFocusRequest, int> {
  /// 历史记录焦点请求 Notifier。
  ///
  /// 用于在窗口弹出时强制触发 UI 层的文本框 Focus 操作。
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

/// 历史记录焦点请求 Notifier。
///
/// 用于在窗口弹出时强制触发 UI 层的文本框 Focus 操作。

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

/// 经过过滤和排序的历史记录流。
///
/// 核心数据 Provider，监听搜索词、搜索模式及存储限制，实时从仓库获取数据库数据。

@ProviderFor(filteredHistory)
final filteredHistoryProvider = FilteredHistoryProvider._();

/// 经过过滤和排序的历史记录流。
///
/// 核心数据 Provider，监听搜索词、搜索模式及存储限制，实时从仓库获取数据库数据。

final class FilteredHistoryProvider
    extends
        $FunctionalProvider<
          AsyncValue<List<HistoryItem>>,
          List<HistoryItem>,
          Stream<List<HistoryItem>>
        >
    with
        $FutureModifier<List<HistoryItem>>,
        $StreamProvider<List<HistoryItem>> {
  /// 经过过滤和排序的历史记录流。
  ///
  /// 核心数据 Provider，监听搜索词、搜索模式及存储限制，实时从仓库获取数据库数据。
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
  $StreamProviderElement<List<HistoryItem>> $createElement(
    $ProviderPointer pointer,
  ) => $StreamProviderElement(pointer);

  @override
  Stream<List<HistoryItem>> create(Ref ref) {
    return filteredHistory(ref);
  }
}

String _$filteredHistoryHash() => r'f80ed23ab93e68ae4bdccf7a8fbf5efda990fe9a';

/// Pin 的历史记录流。
///
/// 仅返回已固定的条目。

@ProviderFor(pinnedHistory)
final pinnedHistoryProvider = PinnedHistoryProvider._();

/// Pin 的历史记录流。
///
/// 仅返回已固定的条目。

final class PinnedHistoryProvider
    extends
        $FunctionalProvider<
          AsyncValue<List<HistoryItem>>,
          List<HistoryItem>,
          Stream<List<HistoryItem>>
        >
    with
        $FutureModifier<List<HistoryItem>>,
        $StreamProvider<List<HistoryItem>> {
  /// Pin 的历史记录流。
  ///
  /// 仅返回已固定的条目。
  PinnedHistoryProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'pinnedHistoryProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$pinnedHistoryHash();

  @$internal
  @override
  $StreamProviderElement<List<HistoryItem>> $createElement(
    $ProviderPointer pointer,
  ) => $StreamProviderElement(pointer);

  @override
  Stream<List<HistoryItem>> create(Ref ref) {
    return pinnedHistory(ref);
  }
}

String _$pinnedHistoryHash() => r'd45786045e5db861dca18e85cf5b159892454260';

/// 非 Pin 的历史记录流。
///
/// 仅返回未固定的条目。

@ProviderFor(unpinnedHistory)
final unpinnedHistoryProvider = UnpinnedHistoryProvider._();

/// 非 Pin 的历史记录流。
///
/// 仅返回未固定的条目。

final class UnpinnedHistoryProvider
    extends
        $FunctionalProvider<
          AsyncValue<List<HistoryItem>>,
          List<HistoryItem>,
          Stream<List<HistoryItem>>
        >
    with
        $FutureModifier<List<HistoryItem>>,
        $StreamProvider<List<HistoryItem>> {
  /// 非 Pin 的历史记录流。
  ///
  /// 仅返回未固定的条目。
  UnpinnedHistoryProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'unpinnedHistoryProvider',
        isAutoDispose: true,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$unpinnedHistoryHash();

  @$internal
  @override
  $StreamProviderElement<List<HistoryItem>> $createElement(
    $ProviderPointer pointer,
  ) => $StreamProviderElement(pointer);

  @override
  Stream<List<HistoryItem>> create(Ref ref) {
    return unpinnedHistory(ref);
  }
}

String _$unpinnedHistoryHash() => r'a899a514ef1f803426245d3fcf5a783ec45d556e';

/// 历史记录交互控制器。
///
/// 整合了所有的业务操作逻辑，如条目点击（选择并自动粘贴）、删除、置顶、
/// 全局键盘事件处理（导航、确认、快捷键操作）以及应用的清理逻辑。
///

@ProviderFor(HistoryController)
final historyControllerProvider = HistoryControllerProvider._();

/// 历史记录交互控制器。
///
/// 整合了所有的业务操作逻辑，如条目点击（选择并自动粘贴）、删除、置顶、
/// 全局键盘事件处理（导航、确认、快捷键操作）以及应用的清理逻辑。
///
final class HistoryControllerProvider
    extends $NotifierProvider<HistoryController, void> {
  /// 历史记录交互控制器。
  ///
  /// 整合了所有的业务操作逻辑，如条目点击（选择并自动粘贴）、删除、置顶、
  /// 全局键盘事件处理（导航、确认、快捷键操作）以及应用的清理逻辑。
  ///
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

String _$historyControllerHash() => r'4844729fab9fec20ead5ec0f957d03e5e53cefdf';

/// 历史记录交互控制器。
///
/// 整合了所有的业务操作逻辑，如条目点击（选择并自动粘贴）、删除、置顶、
/// 全局键盘事件处理（导航、确认、快捷键操作）以及应用的清理逻辑。
///

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
