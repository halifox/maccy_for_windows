import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'package:maccy/core/database/database.dart';

part 'database_provider.g.dart';

/// 提供全局唯一的数据库实例。
///
/// 使用 Riverpod 管理 [AppDatabase] 的生命周期，确保应用全局只有一个数据库连接实例，
/// 并在 Provider 销毁时自动关闭数据库连接。
///
/// [ref] Riverpod 引用，用于监听销毁事件。
///
/// 返回 [AppDatabase] 实例。
@Riverpod(keepAlive: true)
AppDatabase appDatabase(Ref ref) {
  final db = AppDatabase();
  ref.onDispose(() => db.close());
  return db;
}
