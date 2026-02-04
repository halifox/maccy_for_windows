// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'database_provider.dart';

// **************************************************************************
// RiverpodGenerator
// **************************************************************************

// GENERATED CODE - DO NOT MODIFY BY HAND
// ignore_for_file: type=lint, type=warning
/// 提供全局唯一的数据库实例。
///
/// 使用 Riverpod 管理 [AppDatabase] 的生命周期，确保应用全局只有一个数据库连接实例，
/// 并在 Provider 销毁时自动关闭数据库连接。
///
/// [ref] Riverpod 引用，用于监听销毁事件。
///
/// 返回 [AppDatabase] 实例。

@ProviderFor(appDatabase)
final appDatabaseProvider = AppDatabaseProvider._();

/// 提供全局唯一的数据库实例。
///
/// 使用 Riverpod 管理 [AppDatabase] 的生命周期，确保应用全局只有一个数据库连接实例，
/// 并在 Provider 销毁时自动关闭数据库连接。
///
/// [ref] Riverpod 引用，用于监听销毁事件。
///
/// 返回 [AppDatabase] 实例。

final class AppDatabaseProvider
    extends $FunctionalProvider<AppDatabase, AppDatabase, AppDatabase>
    with $Provider<AppDatabase> {
  /// 提供全局唯一的数据库实例。
  ///
  /// 使用 Riverpod 管理 [AppDatabase] 的生命周期，确保应用全局只有一个数据库连接实例，
  /// 并在 Provider 销毁时自动关闭数据库连接。
  ///
  /// [ref] Riverpod 引用，用于监听销毁事件。
  ///
  /// 返回 [AppDatabase] 实例。
  AppDatabaseProvider._()
    : super(
        from: null,
        argument: null,
        retry: null,
        name: r'appDatabaseProvider',
        isAutoDispose: false,
        dependencies: null,
        $allTransitiveDependencies: null,
      );

  @override
  String debugGetCreateSourceHash() => _$appDatabaseHash();

  @$internal
  @override
  $ProviderElement<AppDatabase> $createElement($ProviderPointer pointer) =>
      $ProviderElement(pointer);

  @override
  AppDatabase create(Ref ref) {
    return appDatabase(ref);
  }

  /// {@macro riverpod.override_with_value}
  Override overrideWithValue(AppDatabase value) {
    return $ProviderOverride(
      origin: this,
      providerOverride: $SyncValueProvider<AppDatabase>(value),
    );
  }
}

String _$appDatabaseHash() => r'448adad5717e7b1c0b3ca3ca7e03d0b2116237af';
