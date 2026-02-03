import 'package:riverpod_annotation/riverpod_annotation.dart';
import 'database.dart';

part 'database_provider.g.dart';



/// 提供全局唯一的数据库实例，并在 dispose 时关闭连接

@Riverpod(keepAlive: true)

AppDatabase appDatabase(Ref ref) {

  final db = AppDatabase();

  ref.onDispose(() => db.close());

  return db;

}
