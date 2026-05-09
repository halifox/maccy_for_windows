import 'package:drift/drift.dart';
import 'package:riverpod_annotation/riverpod_annotation.dart';

import 'package:maccy/core/database/database.dart';
import 'package:maccy/core/database/database_provider.dart';
import 'package:maccy/core/services/search_service.dart';
import 'package:maccy/features/settings/providers/settings_provider.dart';

part 'history_repository.g.dart';

/// 历史记录数据仓库。
///
/// 封装了与底层数据库的所有交互逻辑，包括条目的观察流、增删改查以及复杂的搜索过滤。
///
/// 字段说明:
/// [_db] 注入的数据库实例。
/// [_ref] Riverpod 引用，用于读取配置。
class HistoryRepository {

  HistoryRepository(this._db, this._ref);
  final AppDatabase _db;
  final Ref _ref;

  /// 监听并观察数据库中的剪贴板条目。
  ///
  /// 返回一个流，当数据库数据发生变化时自动推送最新的条目列表。支持根据 [query] 进行过滤。
  /// 排序规则：置顶状态 > 用户选择的排序模式（lastCopiedAt/firstCopiedAt/numberOfCopies）。
  ///
  /// [query] 搜索关键词。
  /// [searchMode] 搜索模式（exact, regex, mixed, fuzzy）。
  /// [limit] 最大返回条数。
  Stream<List<ClipboardEntry>> watchEntries({
    String? query,
    String searchMode = 'exact',
    required int limit,
  }) {
    final sortBy = _ref.read(sortByProvider);
    final pinPosition = _ref.read(pinPositionProvider);

    // 构建查询
    final select = _db.select(_db.clipboardEntries);

    // 构建排序规则
    select.orderBy([
      // 1. 置顶项目排序（根据配置在顶部或底部）
      if (pinPosition == 'top')
        (t) => OrderingTerm.desc(t.isPinned),

      // 2. 置顶项目内部排序
      (t) => OrderingTerm.desc(t.pinOrder),

      // 3. 主排序规则
      (t) {
        switch (sortBy) {
          case 'lastCopiedAt':
            return OrderingTerm.desc(t.lastCopiedAt);
          case 'firstCopiedAt':
            return OrderingTerm.asc(t.firstCopiedAt);
          case 'numberOfCopies':
            return OrderingTerm.desc(t.copyCount);
          default:
            return OrderingTerm.desc(t.createdAt);
        }
      },

      // 4. 置顶项目在底部时的排序
      if (pinPosition == 'bottom')
        (t) => OrderingTerm.asc(t.isPinned),
    ]);

    select.limit(limit);

    // 如果没有搜索关键词，直接返回
    if (query == null || query.isEmpty) {
      return select.watch();
    }

    // 有搜索关键词时，使用 SearchService 进行内存过滤
    return select.watch().map((entries) {
      final mode = _parseSearchMode(searchMode);
      return SearchService.search(
        query: query,
        items: entries,
        mode: mode,
      );
    });
  }

  /// 解析搜索模式字符串为枚举。
  SearchMode _parseSearchMode(String mode) {
    switch (mode) {
      case 'exact':
        return SearchMode.exact;
      case 'fuzzy':
        return SearchMode.fuzzy;
      case 'regex':
        return SearchMode.regexp;
      case 'mixed':
        return SearchMode.mixed;
      default:
        return SearchMode.exact;
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

  /// 获取所有固定项。
  ///
  /// 按 pinOrder 升序排列。
  Future<List<ClipboardEntry>> getPinnedItems() async {
    return (_db.select(_db.clipboardEntries)
      ..where((t) => t.isPinned.equals(true))
      ..orderBy([(t) => OrderingTerm.asc(t.pinOrder)]))
      .get();
  }

  /// 根据 pinOrder 获取项目。
  ///
  /// [order] 固定顺序（0-20）。
  Future<ClipboardEntry?> getItemByPinOrder(int order) async {
    return (_db.select(_db.clipboardEntries)
      ..where((t) => t.pinOrder.equals(order)))
      .getSingleOrNull();
  }

  /// 更新固定状态。
  ///
  /// [id] 项目 ID。
  /// [isPinned] 是否固定。
  /// [pinOrder] 固定顺序（0-20），如果 isPinned 为 false 则为 null。
  Future<void> updatePinStatus(int id, bool isPinned, int? pinOrder) async {
    await (_db.update(_db.clipboardEntries)
      ..where((t) => t.id.equals(id)))
      .write(ClipboardEntriesCompanion(
        isPinned: Value(isPinned),
        pinOrder: Value(pinOrder),
      ));
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
  return HistoryRepository(db, ref);
}
