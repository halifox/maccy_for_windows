import 'package:hooks_riverpod/hooks_riverpod.dart';

import 'package:maccy/core/database/database.dart' as db;
import 'package:maccy/features/settings/providers/settings_provider.dart';

/// 排序策略枚举
///
/// 对应 Maccy 的 Sorter.By
enum SortStrategy {
  /// 按最后复制时间排序（默认）
  lastCopiedAt,

  /// 按首次复制时间排序
  firstCopiedAt,

  /// 按复制次数排序
  numberOfCopies,
}

/// 固定项位置枚举
enum PinPosition {
  /// 固定项显示在顶部
  top,

  /// 固定项显示在底部
  bottom,
}

/// 排序服务
///
/// 实现 Maccy 的排序逻辑：
/// 1. 固定项优先（根据 pinPosition 配置）
/// 2. 按选定策略排序未固定项
class SorterService {
  SorterService(this.ref);

  final Ref ref;

  /// 对历史记录列表进行排序
  ///
  /// [items] 待排序的历史记录列表
  /// [strategy] 排序策略（可选，默认使用配置中的策略）
  List<db.ClipboardEntry> sort(
    List<db.ClipboardEntry> items, {
    SortStrategy? strategy,
  }) {
    final sortStrategy = strategy ?? _getCurrentStrategy();
    final pinPosition = _getPinPosition();

    // 分离固定项和未固定项
    final pinnedItems = items.where((item) => item.isPinned).toList();
    final unpinnedItems = items.where((item) => !item.isPinned).toList();

    // 对固定项按 pinOrder 排序
    pinnedItems.sort((a, b) => (a.pinOrder ?? 0).compareTo(b.pinOrder ?? 0));

    // 对未固定项按策略排序
    _sortByStrategy(unpinnedItems, sortStrategy);

    // 根据配置组合结果
    if (pinPosition == PinPosition.top) {
      return [...pinnedItems, ...unpinnedItems];
    } else {
      return [...unpinnedItems, ...pinnedItems];
    }
  }

  /// 按策略排序列表
  void _sortByStrategy(List<db.ClipboardEntry> items, SortStrategy strategy) {
    switch (strategy) {
      case SortStrategy.lastCopiedAt:
        items.sort((a, b) => b.lastCopiedAt.compareTo(a.lastCopiedAt));
        break;

      case SortStrategy.firstCopiedAt:
        items.sort((a, b) => b.firstCopiedAt.compareTo(a.firstCopiedAt));
        break;

      case SortStrategy.numberOfCopies:
        items.sort((a, b) {
          final copiesCompare = b.copyCount.compareTo(a.copyCount);
          // 如果复制次数相同，按最后复制时间排序
          if (copiesCompare == 0) {
            return b.lastCopiedAt.compareTo(a.lastCopiedAt);
          }
          return copiesCompare;
        });
        break;
    }
  }

  /// 获取当前排序策略
  SortStrategy _getCurrentStrategy() {
    final sortBy = ref.read(sortByProvider);
    switch (sortBy) {
      case 'firstCopiedAt':
        return SortStrategy.firstCopiedAt;
      case 'numberOfCopies':
        return SortStrategy.numberOfCopies;
      case 'lastCopiedAt':
      default:
        return SortStrategy.lastCopiedAt;
    }
  }

  /// 获取固定项位置配置
  PinPosition _getPinPosition() {
    final position = ref.read(pinPositionProvider);
    return position == 'bottom' ? PinPosition.bottom : PinPosition.top;
  }

  /// 将排序策略转换为字符串（用于保存配置）
  static String strategyToString(SortStrategy strategy) {
    switch (strategy) {
      case SortStrategy.lastCopiedAt:
        return 'lastCopiedAt';
      case SortStrategy.firstCopiedAt:
        return 'firstCopiedAt';
      case SortStrategy.numberOfCopies:
        return 'numberOfCopies';
    }
  }

  /// 从字符串解析排序策略
  static SortStrategy strategyFromString(String value) {
    switch (value) {
      case 'firstCopiedAt':
        return SortStrategy.firstCopiedAt;
      case 'numberOfCopies':
        return SortStrategy.numberOfCopies;
      case 'lastCopiedAt':
      default:
        return SortStrategy.lastCopiedAt;
    }
  }
}

/// Provider for SorterService
final sorterServiceProvider = Provider<SorterService>((ref) {
  return SorterService(ref);
});
