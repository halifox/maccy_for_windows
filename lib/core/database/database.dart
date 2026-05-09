import 'package:drift/drift.dart';
import 'package:drift_flutter/drift_flutter.dart';
import 'package:sqlite3/sqlite3.dart';

part 'database.g.dart';

/// 剪贴板条目数据库表定义。
///
/// 用于存储从系统剪贴板捕获的所有文本、图片或文件记录。
///
/// 字段说明:
/// [id] 主键 ID，自增。
/// [content] 剪贴板文本内容，设有唯一索引以防重复记录。
/// [type] 内容类型（如 text, image, file），默认为 'text'。
/// [createdAt] 记录创建时间，默认为当前系统时间。
/// [isPinned] 是否置顶标记，默认为 false。
/// [pinOrder] 置顶排序权重，数值越大排序越靠前。
/// [appName] 来源应用程序名称（可选）。
/// [copyCount] 复制次数统计，默认为 1。
/// [firstCopiedAt] 首次复制时间，默认为当前系统时间。
/// [lastCopiedAt] 最后复制时间，默认为当前系统时间。
/// [htmlContent] HTML 格式内容（可选）。
/// [rtfContent] RTF 格式内容（可选）。
class ClipboardEntries extends Table {
  IntColumn get id => integer().autoIncrement()();
  TextColumn get content => text().unique()();
  TextColumn get type => text().withDefault(const Constant('text'))();
  DateTimeColumn get createdAt => dateTime().withDefault(currentDateAndTime)();
  BoolColumn get isPinned => boolean().withDefault(const Constant(false))();
  IntColumn get pinOrder => integer().nullable()();
  TextColumn get appName => text().nullable()();
  IntColumn get copyCount => integer().withDefault(const Constant(1))();
  DateTimeColumn get firstCopiedAt => dateTime().withDefault(currentDateAndTime)();
  DateTimeColumn get lastCopiedAt => dateTime().withDefault(currentDateAndTime)();
  TextColumn get htmlContent => text().nullable()();
  TextColumn get rtfContent => text().nullable()();
}

/// 应用程序数据库类。
///
/// 基于 Drift (Sqlite) 构建，提供剪贴板条目的持久化存储、版本迁移管理、
/// 以及原生自定义函数（如正则表达式支持）的注册。
@DriftDatabase(tables: [ClipboardEntries])
class AppDatabase extends _$AppDatabase {
  /// 构造函数。
  ///
  /// 初始化数据库连接。
  AppDatabase() : super(_openConnection());

  @override
  int get schemaVersion => 10;

  /// 获取数据库迁移策略。
  ///
  /// 负责处理表创建及不同版本间的增量更新逻辑。
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
          await m.createIndex(
            Index(
              'clipboard_entries_content',
              'CREATE UNIQUE INDEX IF NOT EXISTS clipboard_entries_content ON clipboard_entries (content)',
            ),
          );
        }
        if (from < 8) {
          try {
            await m.addColumn(clipboardEntries, clipboardEntries.appName);
          } catch (_) {}
        }
        if (from < 9) {
          try {
            await m.addColumn(clipboardEntries, clipboardEntries.copyCount);
            await m.addColumn(clipboardEntries, clipboardEntries.firstCopiedAt);
            await m.addColumn(clipboardEntries, clipboardEntries.lastCopiedAt);
          } catch (_) {}
        }
        if (from < 10) {
          try {
            await m.addColumn(clipboardEntries, clipboardEntries.htmlContent);
            await m.addColumn(clipboardEntries, clipboardEntries.rtfContent);
          } catch (_) {}
        }
      },
      beforeOpen: (details) async {},
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
