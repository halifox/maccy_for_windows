import 'package:drift/drift.dart';
import 'package:drift_flutter/drift_flutter.dart';
import 'package:sqlite3/sqlite3.dart';

part 'database.g.dart';

/// 剪贴板条目数据库表定义
class ClipboardEntries extends Table {
  /// 主键 ID，自增
  IntColumn get id => integer().autoIncrement()();

  /// 剪贴板内容，唯一索引
  TextColumn get content => text().unique()();

  /// 内容类型（如 text, image, file），默认为 'text'
  TextColumn get type => text().withDefault(const Constant('text'))();

  /// 创建时间，默认为当前系统时间
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();

  /// 是否置顶，默认为 false
  BoolColumn get isPinned => boolean().withDefault(const Constant(false))();

  /// 置顶排序权重
  IntColumn get pinOrder => integer().nullable()();
}

/// 应用程序数据库类
@DriftDatabase(tables: [ClipboardEntries])
class AppDatabase extends _$AppDatabase {
  /// 构造函数，初始化数据库连接
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 7;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (m) async {
        await m.createAll();
      },
      onUpgrade: (m, from, to) async {
        if (from < 5) {
          try {
            await m.addColumn(clipboardEntries, clipboardEntries.isPinned);
          } catch (_) {}
        }
        if (from < 6) {
          try {
            await m.addColumn(clipboardEntries, clipboardEntries.pinOrder);
          } catch (_) {}
        }
        if (from < 7) {
          // 为现有表添加唯一索引以支持 Upsert
          await m.createIndex(Index('clipboard_entries_content', 'CREATE UNIQUE INDEX IF NOT EXISTS clipboard_entries_content ON clipboard_entries (content)'));
        }
      },
      beforeOpen: (details) async {
        // 在打开数据库前注册 REGEXP 函数
      },
    );
  }

  /// 开启数据库连接并配置原生选项
  static QueryExecutor _openConnection() {
    return driftDatabase(
      name: 'clipboard_db',
      native: DriftNativeOptions(
        shareAcrossIsolates: true,
        setup: (db) {
          // 注册正则表达式函数
          db.createFunction(
            functionName: 'regexp',
            argumentCount: AllowedArgumentCount(2),
            deterministic: true,
            directOnly: false,
            function: (args) {
              final pattern = args[0] as String;
              final input = args[1] as String;
              return RegExp(pattern, caseSensitive: false).hasMatch(input) ? 1 : 0;
            },
          );
        },
      ),
    );
  }
}
