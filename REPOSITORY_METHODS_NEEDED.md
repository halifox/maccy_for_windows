# HistoryRepository 需要添加的方法

为了支持新的服务层，需要在 `lib/features/history/repositories/history_repository.dart` 中添加以下方法：

## 1. 固定项相关方法（PinService 需要）

```dart
/// 获取所有固定项
/// 按 pinOrder 升序排列
Future<List<ClipboardEntry>> getPinnedItems() async {
  return (select(db.clipboardEntries)
    ..where((t) => t.isPinned.equals(true))
    ..orderBy([(t) => OrderingTerm(expression: t.pinOrder)]))
    .get();
}

/// 根据 pinOrder 获取项目
Future<ClipboardEntry?> getItemByPinOrder(int order) async {
  return (select(db.clipboardEntries)
    ..where((t) => t.pinOrder.equals(order)))
    .getSingleOrNull();
}

/// 更新固定状态
/// [id] 项目 ID
/// [isPinned] 是否固定
/// [pinOrder] 固定顺序（0-20），如果 isPinned 为 false 则为 null
Future<void> updatePinStatus(int id, bool isPinned, int? pinOrder) async {
  await (update(db.clipboardEntries)
    ..where((t) => t.id.equals(id)))
    .write(ClipboardEntriesCompanion(
      isPinned: Value(isPinned),
      pinOrder: Value(pinOrder),
    ));
}
```

## 2. 使用示例

```dart
// 固定一个项目
final pinService = PinService(repository);
await pinService.pin(item); // 自动分配 pinOrder

// 取消固定
await pinService.unpin(item);

// 使用特定快捷键固定
await pinService.pinWithKey(item, 'b'); // 分配 pinOrder = 0

// 获取所有固定项
final pinnedItems = await pinService.getPinnedItems();

// 根据快捷键获取项目
final item = await pinService.getItemByKey('c'); // pinOrder = 1
```

## 3. 数据库字段说明

- `isPinned`: Boolean - 是否固定
- `pinOrder`: Int? - 固定顺序（0-20 对应快捷键 b-y）
  - 0 = 'b'
  - 1 = 'c'
  - ...
  - 20 = 'y'

## 4. 快速实现

在 `history_repository.dart` 中添加：

```dart
@riverpod
class HistoryRepository extends _$HistoryRepository {
  @override
  FutureOr<void> build() async {
    // 现有初始化代码
  }

  // 添加以下三个方法
  Future<List<ClipboardEntry>> getPinnedItems() async {
    final db = ref.read(databaseProvider);
    return (db.select(db.clipboardEntries)
      ..where((t) => t.isPinned.equals(true))
      ..orderBy([(t) => OrderingTerm(expression: t.pinOrder)]))
      .get();
  }

  Future<ClipboardEntry?> getItemByPinOrder(int order) async {
    final db = ref.read(databaseProvider);
    return (db.select(db.clipboardEntries)
      ..where((t) => t.pinOrder.equals(order)))
      .getSingleOrNull();
  }

  Future<void> updatePinStatus(int id, bool isPinned, int? pinOrder) async {
    final db = ref.read(databaseProvider);
    await (db.update(db.clipboardEntries)
      ..where((t) => t.id.equals(id)))
      .write(ClipboardEntriesCompanion(
        isPinned: Value(isPinned),
        pinOrder: Value(pinOrder),
      ));
  }
}
```

## 5. 注意事项

1. 确保 `ClipboardEntries` 表已经有 `isPinned` 和 `pinOrder` 字段
2. `pinOrder` 应该是可空的整数类型
3. 固定项应该在列表中优先显示（使用 SorterService）
4. 最多支持 21 个固定项（对应 21 个快捷键）
