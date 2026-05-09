// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'window_manager_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 窗口管理器。
///
/// 负责主窗口（剪贴板历史窗口、设置窗口）的生命周期管理，
/// 包括窗口的创建、显示/隐藏切换、窗口位置计算（如光标跟随或托盘跟随）以及失焦自动隐藏逻辑。
///
/// 字段说明:
/// [_isShowing] 标记当前窗口是否处于显示状态。
/// [_lastHideTime] 上次窗口隐藏的时间戳，用于防止触发抖动。
/// [_windowSize] 历史记录窗口的默认尺寸。

@ProviderFor(AppWindowManager)
final appWindowManagerProvider = AppWindowManagerProvider._();

/// 窗口管理器。
///
/// 负责主窗口（剪贴板历史窗口、设置窗口）的生命周期管理，
/// 包括窗口的创建、显示/隐藏切换、窗口位置计算（如光标跟随或托盘跟随）以及失焦自动隐藏逻辑。
///
/// 字段说明:
/// [_isShowing] 标记当前窗口是否处于显示状态。
/// [_lastHideTime] 上次窗口隐藏的时间戳，用于防止触发抖动。
/// [_windowSize] 历史记录窗口的默认尺寸。
final class AppWindowManagerProvider
    extends $AsyncNotifierProvider<AppWindowManager, void> {
  /// 窗口管理器。
  ///
  /// 负责主窗口（剪贴板历史窗口、设置窗口）的生命周期管理，
  /// 包括窗口的创建、显示/隐藏切换、窗口位置计算（如光标跟随或托盘跟随）以及失焦自动隐藏逻辑。
  ///
  /// 字段说明:
  /// [_isShowing] 标记当前窗口是否处于显示状态。
  /// [_lastHideTime] 上次窗口隐藏的时间戳，用于防止触发抖动。
  /// [_windowSize] 历史记录窗口的默认尺寸。
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
}

String _$appWindowManagerHash() => r'134f3a53a29991012d832aa41a6b006c44fad4ab';

/// 窗口管理器。
///
/// 负责主窗口（剪贴板历史窗口、设置窗口）的生命周期管理，
/// 包括窗口的创建、显示/隐藏切换、窗口位置计算（如光标跟随或托盘跟随）以及失焦自动隐藏逻辑。
///
/// 字段说明:
/// [_isShowing] 标记当前窗口是否处于显示状态。
/// [_lastHideTime] 上次窗口隐藏的时间戳，用于防止触发抖动。
/// [_windowSize] 历史记录窗口的默认尺寸。

abstract class _$AppWindowManager extends $AsyncNotifier<void> {
  FutureOr<void> build();
  @$mustCallSuper
  @override
  void runBuild() {
    final ref = this.ref as $Ref<AsyncValue<void>, void>;
    final element =
        ref.element
            as $ClassProviderElement<
              AnyNotifier<AsyncValue<void>, void>,
              AsyncValue<void>,
              Object?,
              Object?
            >;
    element.handleCreate(ref, build);
  }
}
