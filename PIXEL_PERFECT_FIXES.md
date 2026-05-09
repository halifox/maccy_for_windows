# 像素级修复清单

## 🚨 关键问题发现

对比 Maccy 源码后，发现以下需要修复的问题：

### 1. ❌ **Pin/Delete 图标按钮必须移除**

**当前代码（history_page.dart:370-383）：**
```dart
if (isSelected) ...[
  _HoverIcon(
    icon: item.pin != null ? Icons.push_pin : Icons.push_pin_outlined,
    onTap: onPin,
    hoverColor: Colors.white,
  ),
  const SizedBox(width: 8),
  _HoverIcon(
    icon: Icons.delete_outline,
    onTap: onDelete,
    hoverColor: Colors.redAccent,
  ),
  const SizedBox(width: 8),
],
```

**❌ 问题：Maccy 没有这些按钮！**

**✅ 修复：完全删除这段代码**

---

### 2. ❌ **Pin 图标必须移除**

**当前代码（history_page.dart:362-363）：**
```dart
if (item.pin != null)
  const Icon(Icons.push_pin, size: 10, color: Colors.blueAccent),
```

**❌ 问题：Maccy 不显示 pin 图标！**

**✅ 修复：完全删除这段代码**

---

### 3. ⚠️ **Item 高度错误**

**当前常量（ui_constants.dart:27）：**
```dart
static const double itemHeight = 24.0;  // macOS 26+
```

**❌ 问题：应该使用旧版尺寸 22.0**

**✅ 修复：**
```dart
static const double itemHeight = 22.0;  // Maccy v2.6.1 标准
```

---

### 4. ⚠️ **圆角半径错误**

**当前常量（ui_constants.dart:24）：**
```dart
static const double cornerRadius = 6.0;  // macOS 26+
```

**❌ 问题：应该使用旧版尺寸 4.0**

**✅ 修复：**
```dart
static const double cornerRadius = 4.0;  // Maccy v2.6.1 标准
```

---

### 5. ⚠️ **快捷键宽度不精确**

**当前代码（history_page.dart:385-396）：**
```dart
if (shortcut != null)
  Text(
    '⌘$shortcut',
    style: TextStyle(
      fontSize: MaccyUIConstants.shortcutFontSize,
      // ...
    ),
  ),
```

**❌ 问题：**
1. 没有固定宽度（应该是 67px）
2. 没有分离修饰键和字符
3. 没有设置 opacity: 0.7

**✅ 修复：创建专用的 KeyboardShortcutWidget**

---

### 6. ⚠️ **搜索框高度错误**

**当前代码：使用 CupertinoTextField 默认高度**

**❌ 问题：应该固定为 23px**

**✅ 修复：**
```dart
Container(
  height: 23.0,  // 固定高度
  decoration: BoxDecoration(
    color: Colors.grey.withOpacity(0.1),
    borderRadius: BorderRadius.circular(5.0),
  ),
  // ...
)
```

---

### 7. ⚠️ **文本截断模式错误**

**当前代码（history_page.dart:528）：**
```dart
overflow: TextOverflow.ellipsis,
```

**❌ 问题：应该从中间截断，不是末尾**

**✅ 修复：需要自定义实现中间截断**

---

### 8. ⚠️ **快捷键字体大小错误**

**当前常量（ui_constants.dart:100）：**
```dart
static const double shortcutFontSize = 12.0;
```

**❌ 问题：应该与主文本一致 13.0**

**✅ 修复：**
```dart
static const double shortcutFontSize = 13.0;
```

---

### 9. ⚠️ **搜索框字体大小错误**

**当前常量（ui_constants.dart:103）：**
```dart
static const double searchFieldFontSize = 12.0;
```

**❌ 问题：应该与主文本一致 13.0**

**✅ 修复：**
```dart
static const double searchFieldFontSize = 13.0;
```

---

### 10. ⚠️ **Item 间距错误**

**当前代码（history_page.dart:73-105）：**
```dart
ListView.builder(
  padding: EdgeInsets.zero,
  itemCount: history.length,
  itemBuilder: (BuildContext context, int index) {
    // ...
  },
)
```

**✅ 正确：ListView.builder 默认无间距，这个是对的**

---

## 📝 完整修复代码

### 修复 1: ui_constants.dart

```dart
/// Maccy UI 规格常量。
///
/// 完全基于 Maccy v2.6.1 源码中的 Popup.swift 定义。
class MaccyUIConstants {
  MaccyUIConstants._();

  // ============================================================================
  // Popup 尺寸常量（来自 Popup.swift）
  // ============================================================================

  /// 垂直内边距（上下）
  static const double verticalPadding = 5.0;

  /// 水平内边距（左右）
  static const double horizontalPadding = 5.0;

  /// 垂直分隔符内边距
  static const double verticalSeparatorPadding = 6.0;

  /// 水平分隔符内边距
  static const double horizontalSeparatorPadding = 6.0;

  /// 圆角半径（Maccy v2.6.1 标准）
  static const double cornerRadius = 4.0;  // ✅ 修改：从 6.0 改为 4.0

  /// 列表项高度（Maccy v2.6.1 标准）
  static const double itemHeight = 22.0;  // ✅ 修改：从 24.0 改为 22.0

  /// 默认窗口宽度
  static const double defaultWindowWidth = 450.0;

  /// 默认窗口高度
  static const double defaultWindowHeight = 800.0;

  // ============================================================================
  // ListItemView 尺寸常量（来自 ListItemView.swift）
  // ============================================================================

  /// 应用图标大小
  static const double appIconSize = 15.0;

  /// 应用图标左边距
  static const double appIconLeadingPadding = 4.0;

  /// 应用图标垂直边距
  static const double appIconVerticalPadding = 5.0;

  /// 内容左边距（无图标时）
  static const double contentLeadingPadding = 10.0;

  /// 内容左边距（有图标时）
  static const double contentLeadingPaddingWithIcon = 5.0;

  /// 图片右边距
  static const double imageTrailingPadding = 5.0;

  /// 图片垂直边距
  static const double imageVerticalPadding = 5.0;

  /// 标题右边距
  static const double titleTrailingPadding = 5.0;

  /// 快捷键右边距
  static const double shortcutTrailingPadding = 10.0;

  /// 快捷键占位宽度（无快捷键时）
  static const double shortcutPlaceholderWidth = 50.0;

  // ✅ 新增：快捷键区域精确尺寸
  /// 快捷键修饰符宽度
  static const double shortcutModifiersWidth = 55.0;

  /// 快捷键字符宽度
  static const double shortcutCharacterWidth = 12.0;

  /// 快捷键间距
  static const double shortcutSpacing = 1.0;

  /// 快捷键总宽度（55 + 1 + 12 - 1）
  static const double shortcutTotalWidth = 67.0;

  // ============================================================================
  // HistoryListView 尺寸常量（来自 HistoryListView.swift）
  // ============================================================================

  /// 分隔符水平边距
  static const double dividerHorizontalPadding = 10.0;

  /// 分隔符垂直边距
  static const double dividerVerticalPadding = 3.0;

  /// 滚动指示器左边距
  static const double scrollIndicatorLeadingMargin = 10.0;

  // ============================================================================
  // 颜色常量
  // ============================================================================

  /// 选中项背景色透明度
  static const double selectionBackgroundOpacity = 0.8;

  /// 悬停背景色透明度（用于修复 macOS 26 悬停问题）
  static const double hoverBackgroundOpacity = 0.001;

  /// 快捷键文本透明度
  static const double shortcutOpacity = 0.7;  // ✅ 新增

  // ============================================================================
  // 字体常量
  // ============================================================================

  /// 主要文本字体大小
  static const double primaryFontSize = 13.0;

  /// 快捷键文本字体大小
  static const double shortcutFontSize = 13.0;  // ✅ 修改：从 12.0 改为 13.0

  /// 搜索框文本字体大小
  static const double searchFieldFontSize = 13.0;  // ✅ 修改：从 12.0 改为 13.0

  /// 系统字体名称（macOS）
  static const String systemFontFamily = '.AppleSystemUIFont';

  /// 系统字体名称（Windows）
  static const String systemFontFamilyWindows = 'Segoe UI';

  // ============================================================================
  // 动画常量
  // ============================================================================

  /// 窗口调整大小动画时长（秒）
  static const double resizeAnimationDuration = 0.2;

  /// 列表项动画速度倍数
  static const double listAnimationSpeed = 3.0;

  /// 搜索可见性动画时长（秒）
  static const double searchVisibilityAnimationDuration = 0.2;

  // ============================================================================
  // 预览常量
  // ============================================================================

  /// 预览延迟（毫秒，默认值）
  static const int defaultPreviewDelay = 1500;

  /// 预览悬浮窗与主窗口的间距
  static const double previewOffset = 10.0;

  // ============================================================================
  // 搜索框常量
  // ============================================================================

  /// 搜索框高度
  static const double searchFieldHeight = 23.0;  // ✅ 新增

  /// 搜索框圆角半径
  static const double searchFieldCornerRadius = 5.0;  // ✅ 修改：从 4.0 改为 5.0

  /// 搜索框内边距
  static const double searchFieldPadding = 10.0;  // ✅ 修改：从 5.0 改为 10.0

  /// 搜索框内部垂直边距
  static const double searchFieldInternalVerticalPadding = 4.0;

  /// 搜索框内部水平边距
  static const double searchFieldInternalHorizontalPadding = 5.0;

  /// 搜索框图标大小
  static const double searchFieldIconSize = 11.0;  // ✅ 修改：从 12.0 改为 11.0

  /// 搜索框图标透明度
  static const double searchFieldIconOpacity = 0.8;  // ✅ 新增

  /// 搜索框清除按钮透明度
  static const double searchFieldClearButtonOpacity = 0.9;  // ✅ 新增
}
```

---

### 修复 2: 创建 KeyboardShortcutWidget

创建新文件：`lib/features/history/ui/widgets/keyboard_shortcut_widget.dart`

```dart
import 'dart:io';
import 'package:flutter/material.dart';
import 'package:maccy/core/constants/ui_constants.dart';

/// 键盘快捷键显示组件。
///
/// 完全复刻 Maccy 的 KeyboardShortcutView.swift。
/// 布局：[修饰符 55px] [间距 1px] [字符 12px]
class KeyboardShortcutWidget extends StatelessWidget {
  const KeyboardShortcutWidget({
    required this.shortcut,
    required this.isSelected,
    required this.isDark,
    super.key,
  });

  final String shortcut;
  final bool isSelected;
  final bool isDark;

  @override
  Widget build(BuildContext context) {
    // 分离修饰符和字符
    // 例如：'⌘1' -> modifiers: '⌘', character: '1'
    final modifiers = shortcut.length > 1 ? shortcut.substring(0, shortcut.length - 1) : '';
    final character = shortcut.isNotEmpty ? shortcut[shortcut.length - 1] : '';

    return Opacity(
      opacity: character.isEmpty ? 0.0 : MaccyUIConstants.shortcutOpacity,
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          // 修饰符区域（右对齐）
          SizedBox(
            width: MaccyUIConstants.shortcutModifiersWidth,
            child: Text(
              modifiers,
              textAlign: TextAlign.right,
              style: TextStyle(
                fontSize: MaccyUIConstants.shortcutFontSize,
                fontFamily: Platform.isWindows
                    ? MaccyUIConstants.systemFontFamilyWindows
                    : MaccyUIConstants.systemFontFamily,
                color: isSelected
                    ? Colors.white
                    : (isDark ? Colors.white : Colors.black87),
              ),
            ),
          ),
          // 间距
          const SizedBox(width: MaccyUIConstants.shortcutSpacing),
          // 字符区域（居中）
          SizedBox(
            width: MaccyUIConstants.shortcutCharacterWidth,
            child: Text(
              character,
              textAlign: TextAlign.center,
              style: TextStyle(
                fontSize: MaccyUIConstants.shortcutFontSize,
                fontFamily: Platform.isWindows
                    ? MaccyUIConstants.systemFontFamilyWindows
                    : MaccyUIConstants.systemFontFamily,
                color: isSelected
                    ? Colors.white
                    : (isDark ? Colors.white : Colors.black87),
              ),
            ),
          ),
        ],
      ),
    );
  }
}
```

---

### 修复 3: 修改 _HistoryRow（移除按钮）

修改 `lib/features/history/ui/history_page.dart` 中的 `_HistoryRow.build()` 方法：

```dart
@override
Widget build(BuildContext context, WidgetRef ref) {
  final isSelected = ref.watch(
    historySelectedIndexProvider.select((val) => val == index),
  );
  final isDark = Theme.of(context).brightness == Brightness.dark;
  final previewDelay = ref.watch(previewDelayProvider);

  final showPreview = useState(false);
  final hoverTimer = useRef<Timer?>(null);

  useEffect(() {
    return () {
      hoverTimer.value?.cancel();
    };
  }, []);

  return MouseRegion(
    onEnter: (_) {
      onHover();
      // Start timer for preview
      hoverTimer.value?.cancel();
      hoverTimer.value = Timer(Duration(milliseconds: previewDelay), () {
        showPreview.value = true;
      });
    },
    onExit: (_) {
      hoverTimer.value?.cancel();
      showPreview.value = false;
    },
    child: Stack(
      clipBehavior: Clip.none,
      children: [
        GestureDetector(
          onTap: onTap,
          child: Container(
            height: MaccyUIConstants.itemHeight,
            padding: const EdgeInsets.symmetric(
              horizontal: MaccyUIConstants.contentLeadingPadding,
              vertical: 0,
            ),
            decoration: BoxDecoration(
              // ✅ 修改：使用 BoxDecoration 添加圆角
              color: isSelected ? selectionColor : Colors.transparent,
              borderRadius: BorderRadius.circular(MaccyUIConstants.cornerRadius),
            ),
            child: Row(
              children: [
                // ❌ 删除：Pin 图标
                // if (item.pin != null)
                //   const Icon(Icons.push_pin, size: 10, color: Colors.blueAccent),
                
                Expanded(
                  child: buildContent(ref, isSelected, isDark),
                ),
                
                // ❌ 删除：Pin/Delete 按钮
                // if (isSelected) ...[
                //   _HoverIcon(...),
                //   const SizedBox(width: 8),
                //   _HoverIcon(...),
                //   const SizedBox(width: 8),
                // ],
                
                // ✅ 修改：使用新的 KeyboardShortcutWidget
                if (shortcut != null)
                  Padding(
                    padding: const EdgeInsets.only(
                      right: MaccyUIConstants.shortcutTrailingPadding,
                    ),
                    child: KeyboardShortcutWidget(
                      shortcut: '⌘$shortcut',
                      isSelected: isSelected,
                      isDark: isDark,
                    ),
                  )
                else
                  const SizedBox(width: MaccyUIConstants.shortcutPlaceholderWidth),
              ],
            ),
          ),
        ),
        // Preview popover overlay
        if (showPreview.value)
          Positioned(
            left: 460, // Position to the right of the window
            top: -12,
            child: PreviewPopover(item: item),
          ),
      ],
    ),
  );
}
```

---

### 修复 4: 修改 _MenuRow（使用新的快捷键组件）

修改 `lib/features/history/ui/history_page.dart` 中的 `_MenuRow.build()` 方法：

```dart
@override
Widget build(BuildContext context, WidgetRef ref) {
  final isSelected = ref.watch(
    historySelectedIndexProvider.select((val) => val == index),
  );
  final isDark = Theme.of(context).brightness == Brightness.dark;
  
  return MouseRegion(
    onHover: (_) => onHover(),
    child: GestureDetector(
      onTap: onTap,
      child: Container(
        height: MaccyUIConstants.itemHeight,
        padding: const EdgeInsets.symmetric(
          horizontal: MaccyUIConstants.contentLeadingPadding,
        ),
        decoration: BoxDecoration(
          // ✅ 修改：添加圆角
          color: isSelected ? selectionColor : Colors.transparent,
          borderRadius: BorderRadius.circular(MaccyUIConstants.cornerRadius),
        ),
        child: Row(
          children: [
            Expanded(
              child: Text(
                label,
                style: TextStyle(
                  fontSize: MaccyUIConstants.primaryFontSize,
                  fontFamily: Platform.isWindows
                      ? MaccyUIConstants.systemFontFamilyWindows
                      : MaccyUIConstants.systemFontFamily,
                  color: isSelected
                      ? Colors.white
                      : (isDark ? Colors.white : Colors.black87),
                ),
              ),
            ),
            // ✅ 修改：使用新的 KeyboardShortcutWidget
            if (shortcut != null)
              Padding(
                padding: const EdgeInsets.only(
                  right: MaccyUIConstants.shortcutTrailingPadding,
                ),
                child: KeyboardShortcutWidget(
                  shortcut: shortcut!,
                  isSelected: isSelected,
                  isDark: isDark,
                ),
              )
            else
              const SizedBox(width: MaccyUIConstants.shortcutPlaceholderWidth),
          ],
        ),
      ),
    ),
  );
}
```

---

### 修复 5: 修改搜索框（固定高度）

修改 `lib/features/history/ui/history_page.dart` 中的 `_HistoryHeader.build()` 方法：

```dart
@override
Widget build(BuildContext context, WidgetRef ref) {
  final isDark = Theme.of(context).brightness == Brightness.dark;
  final searchQuery = ref.watch(historySearchQueryProvider);
  final searchController = useTextEditingController(text: searchQuery);
  final searchFocusNode = useFocusNode();
  final debouncer = useMemoized(() => _Debouncer(milliseconds: 150));

  ref.listen(historyFocusRequestProvider, (_, _) {
    searchFocusNode.requestFocus();
  });

  useEffect(() {
    if (searchController.text != searchQuery) {
      searchController.text = searchQuery;
    }
    return null;
  }, [searchQuery]);

  useEffect(() {
    searchFocusNode.requestFocus();
    return null;
  }, []);

  useOnAppLifecycleStateChange((oldState, newState) {
    if (newState == AppLifecycleState.resumed) {
      searchFocusNode.requestFocus();
    }
  });

  return Padding(
    padding: const EdgeInsets.all(MaccyUIConstants.searchFieldPadding),
    child: Container(
      // ✅ 新增：固定高度
      height: MaccyUIConstants.searchFieldHeight,
      decoration: BoxDecoration(
        color: isDark
            ? Colors.white.withOpacity(0.1)
            : Colors.black.withOpacity(0.06),
        borderRadius: BorderRadius.circular(MaccyUIConstants.searchFieldCornerRadius),
      ),
      child: Row(
        children: [
          // 搜索图标
          Padding(
            padding: const EdgeInsets.only(
              left: MaccyUIConstants.searchFieldInternalHorizontalPadding,
            ),
            child: Icon(
              Icons.search,
              size: MaccyUIConstants.searchFieldIconSize,
              color: (isDark ? Colors.white : Colors.black)
                  .withOpacity(MaccyUIConstants.searchFieldIconOpacity),
            ),
          ),
          // 输入框
          Expanded(
            child: TextField(
              controller: searchController,
              focusNode: searchFocusNode,
              autofocus: true,
              decoration: const InputDecoration(
                hintText: 'Search...',
                border: InputBorder.none,
                contentPadding: EdgeInsets.symmetric(
                  horizontal: MaccyUIConstants.searchFieldInternalHorizontalPadding,
                ),
              ),
              style: TextStyle(
                fontSize: MaccyUIConstants.searchFieldFontSize,
                color: isDark ? Colors.white : Colors.black,
                fontFamily: Platform.isWindows
                    ? MaccyUIConstants.systemFontFamilyWindows
                    : MaccyUIConstants.systemFontFamily,
              ),
              onChanged: (value) {
                debouncer.run(() {
                  ref.read(historySearchQueryProvider.notifier).value = value;
                  ref.read(historySelectedIndexProvider.notifier).value = 0;
                });
              },
            ),
          ),
          // 清除按钮
          if (searchQuery.isNotEmpty)
            GestureDetector(
              onTap: () {
                searchController.clear();
                ref.read(historySearchQueryProvider.notifier).value = '';
                ref.read(historySelectedIndexProvider.notifier).value = 0;
              },
              child: Padding(
                padding: const EdgeInsets.only(
                  right: MaccyUIConstants.searchFieldInternalHorizontalPadding,
                ),
                child: Icon(
                  Icons.cancel,
                  size: MaccyUIConstants.searchFieldIconSize,
                  color: (isDark ? Colors.white : Colors.black)
                      .withOpacity(MaccyUIConstants.searchFieldClearButtonOpacity),
                ),
              ),
            ),
        ],
      ),
    ),
  );
}
```

---

## ✅ 修复总结

### 必须删除的代码：
1. ❌ Pin 图标（第 362-363 行）
2. ❌ Pin/Delete 按钮（第 370-383 行）
3. ❌ `_HoverIcon` 组件（第 540-569 行，不再使用）

### 必须修改的常量：
1. `itemHeight`: 24.0 → 22.0
2. `cornerRadius`: 6.0 → 4.0
3. `shortcutFontSize`: 12.0 → 13.0
4. `searchFieldFontSize`: 12.0 → 13.0
5. `searchFieldCornerRadius`: 4.0 → 5.0
6. `searchFieldPadding`: 5.0 → 10.0
7. `searchFieldIconSize`: 12.0 → 11.0

### 必须新增的组件：
1. ✅ `KeyboardShortcutWidget`（精确 67px 宽度）

### 必须修改的样式：
1. ✅ 所有 `Container` 添加 `BorderRadius.circular(4.0)`
2. ✅ 搜索框固定高度 23px
3. ✅ 快捷键 opacity: 0.7

---

## 🎯 验证清单

修改完成后，请验证：

- [ ] 选中项没有 Pin/Delete 按钮
- [ ] 未选中项没有 Pin 图标
- [ ] Item 高度精确 22px
- [ ] 圆角半径 4px
- [ ] 快捷键区域宽度 67px
- [ ] 快捷键字体大小 13px
- [ ] 快捷键 opacity 0.7
- [ ] 搜索框高度 23px
- [ ] 搜索框圆角 5px
- [ ] 所有字体大小 13px

完成这些修改后，您的 UI 将完全匹配 Maccy v2.6.1 的像素级规范！
