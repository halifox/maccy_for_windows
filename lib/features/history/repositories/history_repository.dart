import 'dart:convert';
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

  // Maccy 的临时类型列表（不参与去重比较）
  static const _transientTypes = [
    'com.apple.pasteboard.modified',
    'org.p0deje.Maccy.fromMaccy',
    'com.apple.LinkPresentation.metadata',
    'com.apple.pasteboard.customPasteboardData',
    'com.apple.pasteboard.source',
  ];

  /// 监听并观察数据库中的剪贴板条目。
  ///
  /// 返回一个流，当数据库数据发生变化时自动推送最新的条目列表。支持根据 [query] 进行过滤。
  /// 排序规则：置顶状态 > 用户选择的排序模式（lastCopiedAt/firstCopiedAt/numberOfCopies）。
  ///
  /// [query] 搜索关键词。
  /// [searchMode] 搜索模式（exact, regex, mixed, fuzzy）。
  /// [limit] 最大返回条数。
  Stream<List<HistoryItem>> watchEntries({
    String? query,
    String searchMode = 'exact',
    required int limit,
  }) {
    final sortBy = _ref.read(sortByProvider);
    final pinPosition = _ref.read(pinPositionProvider);

    // 构建查询
    final select = _db.select(_db.historyItems);

    // 构建排序规则
    select.orderBy([
      // 1. 置顶项目排序（根据配置在顶部或底部）
      if (pinPosition == 'top')
        (t) => OrderingTerm.desc(t.pin.isNotNull()),

      // 2. 置顶项目内部按 pin 字符排序
      (t) => OrderingTerm.asc(t.pin),

      // 3. 主排序规则
      (t) {
        switch (sortBy) {
          case 'lastCopiedAt':
            return OrderingTerm.desc(t.lastCopiedAt);
          case 'firstCopiedAt':
            return OrderingTerm.asc(t.firstCopiedAt);
          case 'numberOfCopies':
            return OrderingTerm.desc(t.numberOfCopies);
          default:
            return OrderingTerm.desc(t.lastCopiedAt);
        }
      },

      // 4. 置顶项目在底部时的排序
      if (pinPosition == 'bottom')
        (t) => OrderingTerm.asc(t.pin.isNotNull()),
    ]);

    select.limit(limit);

    // 如果没有搜索关键词，直接返回
    if (query == null || query.isEmpty) {
      return select.watch();
    }

    // 有搜索关键词时，使用 SearchService 进行内存过滤
    return select.watch().map((entries) {
      final mode = _parseSearchMode(searchMode);
      return SearchService.searchHistoryItems(
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
    await (_db.delete(_db.historyItems)..where((t) => t.id.equals(id))).go();
  }

  /// 删除数据库中所有的历史条目。
  Future<void> deleteAllEntries() async {
    await _db.delete(_db.historyItems).go();
  }

  /// 删除所有非固定项。
  Future<void> deleteUnpinnedEntries() async {
    await (_db.delete(_db.historyItems)..where((t) => t.pin.isNull())).go();
  }

  /// 添加或更新历史条目（实现 Maccy 的去重逻辑）。
  ///
  /// [contents] 内容数组（多格式）。
  /// [application] 来源应用。
  /// [title] 显示标题。
  Future<void> addOrUpdateEntry({
    required List<HistoryItemContentData> contents,
    String? application,
    required String title,
  }) async {
    final now = DateTime.now();

    // 查找重复项
    final duplicate = await _findDuplicate(contents);

    if (duplicate != null) {
      // 更新现有项
      await (_db.update(_db.historyItems)..where((t) => t.id.equals(duplicate.id)))
          .write(HistoryItemsCompanion(
        lastCopiedAt: Value(now),
        numberOfCopies: Value(duplicate.numberOfCopies + 1),
        application: Value(application),
      ));
    } else {
      // 插入新项
      final itemId = await _db.into(_db.historyItems).insert(
            HistoryItemsCompanion.insert(
              application: Value(application),
              firstCopiedAt: now,
              lastCopiedAt: now,
              numberOfCopies: const Value(1),
              pin: const Value(null),
              title: title,
            ),
          );

      // 插入内容
      for (final content in contents) {
        await _db.into(_db.historyItemContents).insert(
              HistoryItemContentsCompanion.insert(
                itemId: itemId,
                type: content.type,
                value: Value(content.value),
              ),
            );
      }

      // 限制历史大小（不包括固定项）
      await _limitHistorySize();
    }
  }

  /// 查找重复项（实现 Maccy 的 supersedes 逻辑）。
  Future<HistoryItem?> _findDuplicate(List<HistoryItemContentData> newContents) async {
    final allItems = await _db.select(_db.historyItems).get();

    for (final item in allItems) {
      if (await _supersedes(newContents, item.id)) {
        return item;
      }
    }

    return null;
  }

  /// 判断新内容是否与现有项重复（Maccy 的 supersedes 方法）。
  Future<bool> _supersedes(List<HistoryItemContentData> newContents, int existingItemId) async {
    // 获取现有项的所有内容
    final existingContents = await (_db.select(_db.historyItemContents)
          ..where((t) => t.itemId.equals(existingItemId)))
        .get();

    // 过滤掉临时类型
    final nonTransientExisting =
        existingContents.where((c) => !_transientTypes.contains(c.type)).toList();

    // 检查所有非临时类型的现有内容是否都在新内容中
    for (final existingContent in nonTransientExisting) {
      final found = newContents.any((newContent) =>
          newContent.type == existingContent.type &&
          _compareBytes(newContent.value, existingContent.value));

      if (!found) {
        return false;
      }
    }

    return nonTransientExisting.isNotEmpty;
  }

  /// 比较两个字节数组是否相等。
  bool _compareBytes(Uint8List? a, Uint8List? b) {
    if (a == null && b == null) return true;
    if (a == null || b == null) return false;
    if (a.length != b.length) return false;

    for (int i = 0; i < a.length; i++) {
      if (a[i] != b[i]) return false;
    }

    return true;
  }

  /// 限制历史大小（仅删除非固定项）。
  Future<void> _limitHistorySize() async {
    final limit = _ref.read(historyLimitProvider);

    // 获取所有非固定项，按 lastCopiedAt 降序
    final unpinnedItems = await (_db.select(_db.historyItems)
          ..where((t) => t.pin.isNull())
          ..orderBy([(t) => OrderingTerm.desc(t.lastCopiedAt)]))
        .get();

    // 删除超出限制的项
    if (unpinnedItems.length > limit) {
      final toDelete = unpinnedItems.skip(limit);
      for (final item in toDelete) {
        await deleteEntry(item.id);
      }
    }
  }

  /// 获取所有固定项。
  Future<List<HistoryItem>> getPinnedItems() async {
    return (_db.select(_db.historyItems)
          ..where((t) => t.pin.isNotNull())
          ..orderBy([(t) => OrderingTerm.asc(t.pin)]))
        .get();
  }

  /// 获取指定 pin 字符的项。
  Future<HistoryItem?> getItemByPin(String pin) async {
    return (_db.select(_db.historyItems)..where((t) => t.pin.equals(pin))).getSingleOrNull();
  }

  /// 获取下一个可用的 pin 字符。
  Future<String?> getNextAvailablePin() async {
    // Maccy 的可用字符：b-y（排除 a/q/v/w/z）
    const availablePins = [
      'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
      'm', 'n', 'o', 'p', 'r', 's', 't', 'u', 'x', 'y'
    ];

    final pinnedItems = await getPinnedItems();
    final usedPins = pinnedItems.map((e) => e.pin).whereType<String>().toSet();

    for (final pin in availablePins) {
      if (!usedPins.contains(pin)) {
        return pin;
      }
    }

    return null;
  }

  /// 切换指定条目的置顶状态。
  Future<void> togglePin(int id) async {
    final item = await (_db.select(_db.historyItems)..where((t) => t.id.equals(id)))
        .getSingleOrNull();
    if (item == null) return;

    if (item.pin == null) {
      // 设置为固定
      final nextPin = await getNextAvailablePin();
      if (nextPin != null) {
        await (_db.update(_db.historyItems)..where((t) => t.id.equals(id)))
            .write(HistoryItemsCompanion(pin: Value(nextPin)));
      }
    } else {
      // 取消固定
      await (_db.update(_db.historyItems)..where((t) => t.id.equals(id)))
          .write(const HistoryItemsCompanion(pin: Value(null)));
    }
  }

  /// 获取指定项的所有内容。
  Future<List<HistoryItemContent>> getItemContents(int itemId) async {
    return (_db.select(_db.historyItemContents)..where((t) => t.itemId.equals(itemId))).get();
  }
}

/// 内容数据类（用于传递多格式数据）。
class HistoryItemContentData {
  final String type;
  final Uint8List? value;

  HistoryItemContentData({required this.type, this.value});
}

/// 提供全局的历史记录数据仓库实例。
@riverpod
HistoryRepository historyRepository(Ref ref) {
  final db = ref.watch(appDatabaseProvider);
  return HistoryRepository(db, ref);
}
