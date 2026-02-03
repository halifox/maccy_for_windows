import 'package:drift/drift.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import '../../../core/database/database.dart';
import '../../../core/database/database_provider.dart';

part 'history_repository.g.dart';

/// 历史记录数据仓库，负责与数据库交互进行 CRUD 操作
class HistoryRepository {
  final AppDatabase _db;

  /// 构造函数，注入数据库实例
  HistoryRepository(this._db);

  /// 监听并观察数据库中的剪贴板条目，支持搜索和排序
  Stream<List<ClipboardEntry>> watchEntries({String? query, String searchMode = 'fuzzy', required int limit}) {
    final select = _db.select(_db.clipboardEntries);

    if (query != null && query.isNotEmpty) {
      switch (searchMode) {
        case 'exact':
          select.where((t) => t.content.contains(query));
        case 'regex':
          select.where((t) => t.content.regexp(query));
        case 'mixed':
          try {
            RegExp(query);
            select.where((t) => t.content.regexp(query));
          } catch (_) {
            _applyFuzzySearch(select, query);
          }
        case 'fuzzy':
        default:
          _applyFuzzySearch(select, query);
      }
    }

    return (select
          ..orderBy([
            (t) => OrderingTerm.desc(t.isPinned),
            (t) => OrderingTerm.desc(t.pinOrder),
            (t) => OrderingTerm.desc(t.createdAt),
          ])
          ..limit(limit))
        .watch();
  }

  /// 应用模糊搜索逻辑
  void _applyFuzzySearch(SimpleSelectStatement<$ClipboardEntriesTable, ClipboardEntry> select, String query) {
    // 模糊搜索：将输入按空格拆分，要求内容包含所有关键词
    final terms = query.split(RegExp(r'\s+')).where((t) => t.isNotEmpty);
    for (final term in terms) {
      select.where((t) => t.content.contains(term));
    }
  }

  /// 删除指定 ID 的条目
  Future<void> deleteEntry(int id) async {
    await (_db.delete(_db.clipboardEntries)..where((t) => t.id.equals(id))).go();
  }

  /// 删除所有历史条目
  Future<void> deleteAllEntries() async {
    await _db.delete(_db.clipboardEntries).go();
  }

  /// 添加一条新的剪贴板条目
  Future<void> addEntry(String content, {bool isPinned = false}) async {
    await _db.into(_db.clipboardEntries).insert(
          ClipboardEntriesCompanion.insert(
            content: content,
            isPinned: Value(isPinned),
          ),
        );
  }

  /// 切换指定条目的置顶状态
  Future<void> togglePin(int id) async {
    final item = await (_db.select(_db.clipboardEntries)..where((t) => t.id.equals(id))).getSingleOrNull();
    if (item == null) return;

    final willPin = !item.isPinned;

    if (willPin) {
      final query = _db.selectOnly(_db.clipboardEntries)..addColumns([_db.clipboardEntries.pinOrder.max()]);
      final maxPinOrder = await query.map((row) => row.read(_db.clipboardEntries.pinOrder.max())).getSingle();
      final newOrder = (maxPinOrder ?? 0) + 1;

      await (_db.update(_db.clipboardEntries)..where((t) => t.id.equals(id))).write(ClipboardEntriesCompanion(
        isPinned: const Value(true),
        pinOrder: Value(newOrder),
      ));
    } else {
      await (_db.update(_db.clipboardEntries)..where((t) => t.id.equals(id))).write(const ClipboardEntriesCompanion(
        isPinned: Value(false),
        pinOrder: Value(null),
      ));
    }
  }
}

/// 提供全局的历史记录数据仓库实例
@riverpod
HistoryRepository historyRepository(Ref ref) {
  final db = ref.watch(appDatabaseProvider);
  return HistoryRepository(db);
}
