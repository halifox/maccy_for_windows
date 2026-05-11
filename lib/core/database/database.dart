import 'package:drift/drift.dart';
import 'package:drift_flutter/drift_flutter.dart';
import 'package:sqlite3/sqlite3.dart';

part 'database.g.dart';

/// 历史条目主表（对应 Maccy 的 HistoryItem）。
///
/// 存储剪贴板条目的元数据和关系。
///
/// 字段说明:
/// [id] 主键 ID，自增。
/// [application] 来源应用程序的 bundle ID 或进程名（可选）。
/// [firstCopiedAt] 首次复制时间。
/// [lastCopiedAt] 最后一次复制时间。
/// [numberOfCopies] 复制次数统计，默认为 1。
/// [pin] 固定快捷键，单字符 'b'-'y'（排除 a/q/v/w/z），null 表示未固定。
/// [title] 显示标题。
class HistoryItems extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get application => text().nullable()();
  DateTimeColumn get firstCopiedAt => dateTime()();
  DateTimeColumn get lastCopiedAt => dateTime()();
  IntColumn get numberOfCopies => integer().withDefault(const Constant(1))();
  TextColumn get pin => text().nullable().withLength(min: 1, max: 1)();
  TextColumn get title => text()();
}

/// 历史条目内容表（对应 Maccy 的 HistoryItemContent）。
///
/// 存储单个剪贴板条目的多格式数据（文本、HTML、RTF、图片等）。
/// 一个 HistoryItem 可以有多个 HistoryItemContent。
///
/// 字段说明:
/// [id] 主键 ID，自增。
/// [itemId] 外键，关联到 HistoryItems 表，级联删除。
/// [type] 内容类型标识符（如 'text/plain', 'text/html', 'image/png', 'file'）。
/// [value] 二进制数据内容。
class HistoryItemContents extends Table {
  IntColumn get id => integer().autoIncrement()();
  IntColumn get itemId => integer().references(HistoryItems, #id, onDelete: KeyAction.cascade)();
  TextColumn get type => text()();
  BlobColumn get value => blob().nullable()();
}

/// 应用程序数据库类。
///
/// 基于 Drift (Sqlite) 构建，提供剪贴板条目的持久化存储、版本迁移管理、
/// 以及原生自定义函数（如正则表达式支持）的注册。
@DriftDatabase(tables: [HistoryItems, HistoryItemContents])
class AppDatabase extends _$AppDatabase {
  /// 构造函数。
  ///
  /// 初始化数据库连接。
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 12;

  @override
  MigrationStrategy get migration {
    return MigrationStrategy(
      onCreate: (m) async {
        await m.createAll();
      },
      onUpgrade: (m, from, to) async {
        if (from < 12) {
          // 从版本 11 升级到 12：移除 title 字段的 203 字符限制
          // 重建 history_items 表来移除 CHECK 约束
        }
      },
    );
  }

  /// 开启数据库连接并配置原生选项。
  ///
  /// 使用 [driftDatabase] 创建跨 Isolate 共享的数据库连接，
  /// 并为 SQLite 注册自定义的 'regexp' 函数以支持正则搜索。
  ///
  /// 返回 [QueryExecutor] 数据库执行器。
  static QueryExecutor _openConnection() {
    return driftDatabase(
      name: 'clipboard_db',
      native: DriftNativeOptions(
        shareAcrossIsolates: true,
        setup: (db) {
          db.createFunction(
            functionName: 'regexp',
            argumentCount: const AllowedArgumentCount(2),
            deterministic: true,
            directOnly: false,
            function: (args) {
              final pattern = args[0] as String;
              final input = args[1] as String;
              return RegExp(pattern, caseSensitive: false).hasMatch(input)
                  ? 1
                  : 0;
            },
          );
        },
      ),
    );
  }
}
