import 'package:drift/drift.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'package:haliclip/core/database/database.dart';
import 'package:haliclip/core/database/database_provider.dart';

part 'history_repository.g.dart';

/// 历史记录数据仓库。
///
/// 封装了与底层数据库的所有交互逻辑，包括条目的观察流、增删改查以及复杂的搜索过滤。
///
/// 字段说明:
/// [_db] 注入的数据库实例。
class HistoryRepository {

  HistoryRepository(this._db);
  final AppDatabase _db;

  /// 监听并观察数据库中的剪贴板条目。
  ///
  /// 返回一个流，当数据库数据发生变化时自动推送最新的条目列表。支持根据 [query] 进行过滤。
  /// 排序权重依次为：置顶状态 > 置顶排序权重 > 创建时间。
  ///
  /// [query] 搜索关键词。
  /// [searchMode] 搜索模式（exact, regex, mixed, fuzzy）。
  /// [limit] 最大返回条数。
  Stream<List<ClipboardEntry>> watchEntries({
    String? query,
    String searchMode = 'fuzzy',
    required int limit,
  }) {
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

  /// 应用模糊搜索逻辑。
  ///
  /// 将 [query] 按空格拆分为多个术语，并要求内容必须同时包含所有术语。
  ///
  /// [select] Drift 查询构建器。
  /// [query] 原始查询字符串。
  void _applyFuzzySearch(
    SimpleSelectStatement<$ClipboardEntriesTable, ClipboardEntry> select,
    String query,
  ) {
    final terms = query.split(RegExp(r'\s+')).where((t) => t.isNotEmpty);
    for (final term in terms) {
      select.where((t) => t.content.contains(term));
    }
  }

  /// 删除指定 ID 的剪贴板条目。
  ///
  /// [id] 条目在数据库中的主键。
  Future<void> deleteEntry(int id) async {
    await (_db.delete(
      _db.clipboardEntries,
    )..where((t) => t.id.equals(id))).go();
  }

  /// 删除数据库中所有的历史条目。
  Future<void> deleteAllEntries() async {
    await _db.delete(_db.clipboardEntries).go();
  }

  /// 手动向数据库添加一条新的剪贴板条目。
  ///
  /// [content] 文本内容。
  /// [isPinned] 是否初始化为置顶。
  Future<void> addEntry(String content, {bool isPinned = false}) async {
    await _db
        .into(_db.clipboardEntries)
        .insert(
          ClipboardEntriesCompanion.insert(
            content: content,
            isPinned: Value(isPinned),
          ),
        );
  }

  /// 切换指定条目的置顶状态。
  ///
  /// 若设为置顶，则自动计算新的排序权重（max + 1）；若取消置顶，则清空权重。
  ///
  /// [id] 条目主键。
  Future<void> togglePin(int id) async {
    final item = await (_db.select(
      _db.clipboardEntries,
    )..where((t) => t.id.equals(id))).getSingleOrNull();
    if (item == null) return;

    final willPin = !item.isPinned;

    if (willPin) {
      final query = _db.selectOnly(_db.clipboardEntries)
        ..addColumns([_db.clipboardEntries.pinOrder.max()]);
      final maxPinOrder = await query
          .map((row) => row.read(_db.clipboardEntries.pinOrder.max()))
          .getSingle();
      final newOrder = (maxPinOrder ?? 0) + 1;

      await (_db.update(
        _db.clipboardEntries,
      )..where((t) => t.id.equals(id))).write(
        ClipboardEntriesCompanion(
          isPinned: const Value(true),
          pinOrder: Value(newOrder),
        ),
      );
    } else {
      await (_db.update(
        _db.clipboardEntries,
      )..where((t) => t.id.equals(id))).write(
        const ClipboardEntriesCompanion(
          isPinned: Value(false),
          pinOrder: Value(null),
        ),
      );
    }
  }
}

/// 提供全局的历史记录数据仓库实例。
@riverpod
HistoryRepository historyRepository(Ref ref) {
  final db = ref.watch(appDatabaseProvider);
  return HistoryRepository(db);
}
