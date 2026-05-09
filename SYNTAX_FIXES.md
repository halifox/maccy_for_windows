# 语法错误修复报告

## 已修复的问题

### 1. HistoryItem → ClipboardEntry
- 项目使用的数据模型是 `ClipboardEntry`，不是 `HistoryItem`
- 已更新所有服务类使用正确的类型

### 2. Win32 API 字符串处理
- `calloc<WCHAR>` → `calloc<Uint16>` + `.cast<Utf16>()`
- `toDartString` → `toDartString()`（需要调用方法）
- 修复了 `GetModuleFileNameEx` 和 `GetWindowText` 的参数类型

### 3. ClipboardEntry 字段映射
- `item.title` → `item.content`（数据库字段名）
- `item.pin` → `item.isPinned` + `item.pinOrder`
- `item.numberOfCopies` → `item.copyCount`

### 4. 移除未使用的导入
- 移除了 `paste_service.dart` 中未使用的 win32 导入

## 剩余问题

### PinService 需要的 Repository 方法
以下方法需要在 `HistoryRepository` 中实现：

```dart
// lib/features/history/repositories/history_repository.dart

// 1. 获取固定项
Future<List<ClipboardEntry>> getPinnedItems() async {
  return (select(clipboardEntries)
    ..where((t) => t.isPinned.equals(true))
    ..orderBy([(t) => OrderingTerm(expression: t.pinOrder)]))
    .get();
}

// 2. 按 pinOrder 查找项目
Future<ClipboardEntry?> getItemByPin(int pinOrder) async {
  return (select(clipboardEntries)
    ..where((t) => t.pinOrder.equals(pinOrder)))
    .getSingleOrNull();
}

// 3. 更新固定状态
Future<void> updatePin(int id, int? pinOrder) async {
  await (update(clipboardEntries)
    ..where((t) => t.id.equals(id)))
    .write(ClipboardEntriesCompanion(
      isPinned: Value(pinOrder != null),
      pinOrder: Value(pinOrder),
    ));
}
```

### PinService 需要调整
由于数据模型使用 `pinOrder` (int) 而不是 `pin` (String)，需要调整：

1. `availableKeys` 改为数字 0-20
2. 或者保持字母键，但需要映射到数字

建议方案：
```dart
// 字母 -> 数字映射
static const keyToOrder = {
  'b': 0, 'c': 1, 'd': 2, 'e': 3, 'f': 4,
  'g': 5, 'h': 6, 'i': 7, 'j': 8, 'k': 9,
  'l': 10, 'm': 11, 'n': 12, 'o': 13, 'p': 14,
  'r': 15, 's': 16, 't': 17, 'u': 18, 'x': 19,
  'y': 20,
};
```

## 下一步行动

1. 在 `HistoryRepository` 中添加缺失的方法
2. 调整 `PinService` 以使用 `pinOrder` 而不是 `pin`
3. 删除或修复 `screen_service.dart`（如果不需要）
4. 删除或修复 `foreground_app_service.dart`（已有 `app_identifier_service.dart`）
5. 修复 `search_service.dart` 中的类型错误

## 建议

考虑简化实现：
- 使用现有的 `ClipboardEntry` 模型
- 固定项使用 `isPinned` + `pinOrder` 字段
- 快捷键映射在 UI 层处理，不存储在数据库
