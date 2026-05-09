# Maccy UI 像素级复刻规范

## ✅ 核心确认：无 Pin/Delete 按钮

**您说得完全正确！** Maccy 的弹窗界面中 **没有任何可见的 Pin 或 Delete 图标按钮**。

所有操作都通过 **纯键盘快捷键** 实现：
- **Pin/Unpin**: `Cmd+P`
- **Delete**: `Cmd+Delete` 或 `Cmd+Backspace`
- **Select**: `Enter` 或 `Cmd+数字` 或 `Cmd+字母`

---

## 一、整体布局结构

```
┌─────────────────────────────────────────────┐
│  [Header: 搜索栏]                            │  ← 高度: 22px + 8px padding
├─────────────────────────────────────────────┤
│  [Pinned Items]                             │  ← 固定条目（可选）
│  ─────────────────────────────────────────  │  ← 分隔线（仅当有 pin 时）
│  [Scrollable Unpinned Items]                │  ← 可滚动区域
│                                             │
│                                             │
├─────────────────────────────────────────────┤
│  [Footer: 操作按钮]                          │  ← 高度: 22px + 12px padding
└─────────────────────────────────────────────┘
```

---

## 二、尺寸常量（来自 `Popup.swift`）

### 2.1 全局常量

```dart
// Popup 窗口
static const double verticalPadding = 5.0;      // 上下内边距
static const double horizontalPadding = 5.0;    // 左右内边距
static const double cornerRadius = 4.0;         // 圆角半径（macOS 26.0 为 6.0）

// Item 高度
static const double itemHeight = 22.0;          // 单个条目高度（macOS 26.0 为 24.0）

// 分隔线间距
static const double verticalSeparatorPadding = 6.0;
static const double horizontalSeparatorPadding = 6.0;

// 默认窗口尺寸
static const double defaultWidth = 450.0;
static const double defaultHeight = 800.0;
```

### 2.2 Header 尺寸（`HeaderView.swift`）

```dart
// 搜索栏
static const double searchFieldHeight = 23.0;   // 搜索框高度
static const double headerHeight = 22.0 + 3.0;  // itemHeight + 3px
static const double headerHorizontalPadding = 10.0;
static const double headerBottomPadding = 5.0;  // 搜索栏可见时
static const double headerBottomPaddingHidden = 2.0;  // 搜索栏隐藏时
```

### 2.3 Footer 尺寸（`FooterView.swift`）

```dart
// 底部栏
static const double footerDividerHorizontalPadding = 10.0;
static const double footerDividerVerticalPadding = 6.0;
```

---

## 三、ListItemView 详细规范（核心组件）

### 3.1 布局结构（`ListItemView.swift:19-87`）

```
┌────────────────────────────────────────────────────────────┐
│ [AppIcon] [Spacing] [AccessoryImage/Image] [Title] [Shortcut] │
│   15x15      5/10px      varies              flex      67px   │
└────────────────────────────────────────────────────────────┘
```

### 3.2 精确尺寸

```dart
class ListItemDimensions {
  // 应用图标（可选，默认隐藏）
  static const double appIconSize = 15.0;
  static const double appIconLeadingPadding = 4.0;
  static const double appIconVerticalPadding = 5.0;
  
  // 间距
  static const double spacingWithIcon = 5.0;     // 有图标时
  static const double spacingWithoutIcon = 10.0; // 无图标时
  
  // 图片/颜色标识
  static const double accessoryImageTrailingPadding = 5.0;
  static const double accessoryImageVerticalPadding = 5.0;
  
  // 标题
  static const double titleTrailingPadding = 5.0;
  
  // 快捷键提示区域
  static const double shortcutWidth = 67.0;      // 55px (modifiers) + 1px + 12px (character) - 1px
  static const double shortcutTrailingPadding = 10.0;
  static const double shortcutEmptyWidth = 50.0; // 无快捷键时的占位
  
  // 整体高度
  static const double minHeight = 22.0;          // Popup.itemHeight
}
```

### 3.3 颜色规范（`ListItemView.swift:71-74`）

```dart
class ListItemColors {
  // 前景色
  static const Color foregroundSelected = Colors.white;
  static const Color foregroundNormal = Colors.black87;  // .primary
  
  // 背景色
  static Color backgroundSelected = Colors.blue.withOpacity(0.8);  // accentColor.opacity(0.8)
  static Color backgroundNormal = Colors.white.withOpacity(0.001); // 几乎透明（用于 hover 检测）
  
  // 圆角
  static const double cornerRadius = 4.0;  // Popup.cornerRadius
}
```

### 3.4 文本样式

```dart
class ListItemTextStyle {
  // 标题文本
  static const TextStyle title = TextStyle(
    fontSize: 13.0,           // macOS 系统默认字体大小
    fontWeight: FontWeight.normal,
    overflow: TextOverflow.ellipsis,
    // 截断模式: middle（从中间截断）
  );
  
  // 快捷键文本
  static const TextStyle shortcut = TextStyle(
    fontSize: 13.0,
    fontWeight: FontWeight.normal,
    color: Colors.black54,    // opacity: 0.7
  );
}
```

---

## 四、KeyboardShortcutView 规范（`KeyboardShortcutView.swift`）

### 4.1 布局结构

```
┌──────────────────────────────┐
│ [Modifiers] [Character]      │
│   55px         12px           │
│   右对齐       居中            │
└──────────────────────────────┘
```

### 4.2 精确尺寸

```dart
class KeyboardShortcutDimensions {
  static const double modifiersWidth = 55.0;
  static const double characterWidth = 12.0;
  static const double spacing = 1.0;
  static const double opacity = 0.7;
  static const double opacityEmpty = 0.0;  // 无快捷键时完全透明
}
```

### 4.3 修饰键符号映射

```dart
// macOS 标准符号
const Map<String, String> modifierSymbols = {
  'command': '⌘',
  'shift': '⇧',
  'option': '⌥',
  'control': '⌃',
};
```

---

## 五、SearchFieldView 规范（`SearchFieldView.swift`）

### 5.1 布局结构

```
┌────────────────────────────────────────────┐
│ [🔍] [TextField..................] [✕]    │
│  11px   flex                        11px   │
└────────────────────────────────────────────┘
```

### 5.2 精确尺寸

```dart
class SearchFieldDimensions {
  static const double height = 23.0;
  static const double cornerRadius = 5.0;
  
  // 图标
  static const double iconSize = 11.0;
  static const double iconLeadingPadding = 5.0;
  static const double iconTrailingPadding = 5.0;
  static const double iconOpacity = 0.8;
  
  // 清除按钮
  static const double clearButtonSize = 11.0;
  static const double clearButtonTrailingPadding = 5.0;
  static const double clearButtonOpacity = 0.9;
}
```

### 5.3 颜色规范

```dart
class SearchFieldColors {
  static Color background = Colors.grey.withOpacity(0.1);  // secondary.opacity(0.1)
  static const Color icon = Colors.black54;
  static const Color text = Colors.black87;
  static const Color placeholder = Colors.black38;
}
```

---

## 六、HeaderView 规范（`HeaderView.swift`）

### 6.1 布局结构

```
┌────────────────────────────────────────────┐
│  Maccy  [SearchField...................]   │
│  灰色    占据剩余空间                       │
└────────────────────────────────────────────┘
```

### 6.2 精确尺寸

```dart
class HeaderDimensions {
  static const double height = 22.0 + 3.0;  // itemHeight + 3px
  static const double horizontalPadding = 10.0;
  static const double bottomPadding = 5.0;
  static const double bottomPaddingHidden = 2.0;  // 搜索栏隐藏时防止条目透出
  
  // "Maccy" 标题
  static const TextStyle titleStyle = TextStyle(
    fontSize: 13.0,
    color: Colors.black54,  // .secondary
  );
}
```

---

## 七、FooterView 规范（`FooterView.swift`）

### 7.1 布局结构

```
┌────────────────────────────────────────────┐
│  ─────────────────────────────────────────  │  ← Divider
│  Clear                              ⌘⌫      │  ← FooterItem (Clear)
│  Clear All                       ⌥⌘⌫      │  ← FooterItem (Clear All, 按住 Option 时显示)
│  Preferences                        ⌘,      │  ← FooterItem (Preferences)
│  Quit                               ⌘Q      │  ← FooterItem (Quit)
└────────────────────────────────────────────┘
```

### 7.2 精确尺寸

```dart
class FooterDimensions {
  static const double dividerHorizontalPadding = 10.0;
  static const double dividerVerticalPadding = 6.0;
  
  // FooterItem 使用与 ListItemView 相同的高度和样式
  static const double itemHeight = 22.0;
}
```

### 7.3 特殊交互

```dart
// Clear 和 Clear All 的切换逻辑（FooterView.swift:13-51）
// 1. 默认显示 "Clear" (⌘⌫)
// 2. 按住 Option 键时，切换为 "Clear All" (⌥⌘⌫)
// 3. 使用 ZStack 叠加，通过 opacity 控制显示
```

---

## 八、HistoryListView 规范（`HistoryListView.swift`）

### 8.1 布局结构

```dart
// Pin 在顶部时
VStack(spacing: 0) {
  // Pinned Items (LazyVStack)
  ForEach(pinnedItems) { item in
    HistoryItemView(item: item)
  }
  
  // 分隔线（仅当有 pin 且有搜索结果时）
  if (showPinsSeparator) {
    Divider()
      .padding(horizontal: 10, vertical: 3)
  }
  
  // Scrollable Unpinned Items
  ScrollView {
    LazyVStack(spacing: 0) {
      ForEach(unpinnedItems) { item in
        HistoryItemView(item: item)
      }
    }
  }
}
```

### 8.2 精确尺寸

```dart
class HistoryListDimensions {
  static const double itemSpacing = 0.0;  // 条目之间无间距
  static const double scrollIndicatorLeadingMargin = 10.0;
  
  // 分隔线
  static const double separatorHorizontalPadding = 10.0;
  static const double separatorVerticalPadding = 3.0;
}
```

---

## 九、完整的 Flutter Widget 实现示例

### 9.1 ListItemWidget（核心组件）

```dart
class ListItemWidget extends StatelessWidget {
  final String title;
  final String? appIconPath;
  final Uint8List? imageData;
  final Color? accessoryColor;
  final List<KeyShortcut> shortcuts;
  final bool isSelected;
  final VoidCallback onTap;
  final VoidCallback onHover;

  const ListItemWidget({
    required this.title,
    this.appIconPath,
    this.imageData,
    this.accessoryColor,
    required this.shortcuts,
    required this.isSelected,
    required this.onTap,
    required this.onHover,
    Key? key,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MouseRegion(
      onEnter: (_) => onHover(),
      child: GestureDetector(
        onTap: onTap,
        child: Container(
          height: 22.0,  // Popup.itemHeight
          decoration: BoxDecoration(
            color: isSelected 
              ? Colors.blue.withOpacity(0.8)  // accentColor.opacity(0.8)
              : Colors.white.withOpacity(0.001),
            borderRadius: BorderRadius.circular(4.0),  // Popup.cornerRadius
          ),
          child: Row(
            children: [
              // 应用图标（可选）
              if (appIconPath != null) ...[
                Padding(
                  padding: const EdgeInsets.only(left: 4.0, top: 5.0, bottom: 5.0),
                  child: Image.asset(
                    appIconPath!,
                    width: 15.0,
                    height: 15.0,
                  ),
                ),
                const SizedBox(width: 5.0),
              ] else
                const SizedBox(width: 10.0),
              
              // 颜色标识或图片
              if (accessoryColor != null)
                Padding(
                  padding: const EdgeInsets.only(right: 5.0, top: 5.0, bottom: 5.0),
                  child: Container(
                    width: 12.0,
                    height: 12.0,
                    decoration: BoxDecoration(
                      color: accessoryColor,
                      shape: BoxShape.circle,
                    ),
                  ),
                )
              else if (imageData != null)
                Padding(
                  padding: const EdgeInsets.only(right: 5.0, top: 5.0, bottom: 5.0),
                  child: Image.memory(
                    imageData!,
                    height: 12.0,
                  ),
                ),
              
              // 标题
              if (imageData == null)
                Expanded(
                  child: Padding(
                    padding: const EdgeInsets.only(right: 5.0),
                    child: Text(
                      title,
                      maxLines: 1,
                      overflow: TextOverflow.ellipsis,
                      style: TextStyle(
                        fontSize: 13.0,
                        color: isSelected ? Colors.white : Colors.black87,
                      ),
                    ),
                  ),
                )
              else
                const Spacer(),
              
              // 快捷键提示
              if (shortcuts.isNotEmpty)
                Padding(
                  padding: const EdgeInsets.only(right: 10.0),
                  child: KeyboardShortcutWidget(shortcuts: shortcuts),
                )
              else
                const SizedBox(width: 50.0),
            ],
          ),
        ),
      ),
    );
  }
}
```

### 9.2 KeyboardShortcutWidget

```dart
class KeyboardShortcutWidget extends StatelessWidget {
  final List<KeyShortcut> shortcuts;

  const KeyboardShortcutWidget({
    required this.shortcuts,
    Key? key,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    if (shortcuts.isEmpty) {
      return const SizedBox.shrink();
    }

    final shortcut = shortcuts.first;
    final modifiers = _formatModifiers(shortcut.modifierFlags);
    final character = shortcut.character;

    return Opacity(
      opacity: 0.7,
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          SizedBox(
            width: 55.0,
            child: Text(
              modifiers,
              textAlign: TextAlign.right,
              style: const TextStyle(fontSize: 13.0),
            ),
          ),
          const SizedBox(width: 1.0),
          SizedBox(
            width: 12.0,
            child: Text(
              character,
              textAlign: TextAlign.center,
              style: const TextStyle(fontSize: 13.0),
            ),
          ),
        ],
      ),
    );
  }

  String _formatModifiers(Set<ModifierKey> flags) {
    final symbols = <String>[];
    if (flags.contains(ModifierKey.control)) symbols.add('⌃');
    if (flags.contains(ModifierKey.alt)) symbols.add('⌥');
    if (flags.contains(ModifierKey.shift)) symbols.add('⇧');
    if (flags.contains(ModifierKey.meta)) symbols.add('⌘');
    return symbols.join('');
  }
}
```

### 9.3 SearchFieldWidget

```dart
class SearchFieldWidget extends StatelessWidget {
  final String placeholder;
  final String query;
  final ValueChanged<String> onChanged;
  final VoidCallback onClear;
  final VoidCallback onSubmit;

  const SearchFieldWidget({
    required this.placeholder,
    required this.query,
    required this.onChanged,
    required this.onClear,
    required this.onSubmit,
    Key? key,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Container(
      height: 23.0,
      decoration: BoxDecoration(
        color: Colors.grey.withOpacity(0.1),
        borderRadius: BorderRadius.circular(5.0),
      ),
      child: Row(
        children: [
          const Padding(
            padding: EdgeInsets.only(left: 5.0),
            child: Icon(
              Icons.search,
              size: 11.0,
              color: Colors.black54,
            ),
          ),
          Expanded(
            child: TextField(
              controller: TextEditingController(text: query),
              onChanged: onChanged,
              onSubmitted: (_) => onSubmit(),
              decoration: InputDecoration(
                hintText: placeholder,
                border: InputBorder.none,
                contentPadding: EdgeInsets.zero,
              ),
              style: const TextStyle(fontSize: 13.0),
            ),
          ),
          if (query.isNotEmpty)
            GestureDetector(
              onTap: onClear,
              child: const Padding(
                padding: EdgeInsets.only(right: 5.0),
                child: Icon(
                  Icons.cancel,
                  size: 11.0,
                  color: Colors.black54,
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

## 十、关键交互行为

### 10.1 Hover 效果（`ListItemView.swift:76-84`）

```dart
// 鼠标悬停时：
// 1. 如果不在键盘导航模式：立即选中
// 2. 如果在键盘导航模式：记录悬停项，等待键盘导航结束后选中
```

### 10.2 选中状态切换

```dart
// 选中时：
// - 背景色: Colors.blue.withOpacity(0.8)
// - 文字色: Colors.white
// - 圆角: 4.0

// 未选中时：
// - 背景色: Colors.white.withOpacity(0.001)  // 几乎透明
// - 文字色: Colors.black87
```

### 10.3 文本截断（`ListItemTitleView.swift:11-12`）

```dart
// 截断模式: TextOverflow.ellipsis
// 截断位置: 中间（middle truncation）
// 行数限制: 1
```

---

## 十一、完整的尺寸对照表

| 组件 | 属性 | 值 | 来源文件 |
|-----|------|----|----|
| **Popup** | verticalPadding | 5.0 | Popup.swift:22 |
| | horizontalPadding | 5.0 | Popup.swift:23 |
| | cornerRadius | 4.0 | Popup.swift:27-31 |
| | itemHeight | 22.0 | Popup.swift:33-37 |
| **ListItem** | minHeight | 22.0 | ListItemView.swift:68 |
| | appIconSize | 15.0 | ListItemView.swift:26 |
| | appIconLeadingPadding | 4.0 | ListItemView.swift:29 |
| | spacingWithIcon | 5.0 | ListItemView.swift:34 |
| | spacingWithoutIcon | 10.0 | ListItemView.swift:34 |
| | titleTrailingPadding | 5.0 | ListItemView.swift:50 |
| | shortcutTrailingPadding | 10.0 | ListItemView.swift:62 |
| | shortcutEmptyWidth | 50.0 | ListItemView.swift:65 |
| **KeyboardShortcut** | modifiersWidth | 55.0 | KeyboardShortcutView.swift:20 |
| | characterWidth | 12.0 | KeyboardShortcutView.swift:21 |
| | spacing | 1.0 | KeyboardShortcutView.swift:19 |
| | opacity | 0.7 | KeyboardShortcutView.swift:24 |
| **SearchField** | height | 23.0 | SearchFieldView.swift:14 |
| | cornerRadius | 5.0 | SearchFieldView.swift:11 |
| | iconSize | 11.0 | SearchFieldView.swift:18 |
| | iconLeadingPadding | 5.0 | SearchFieldView.swift:19 |
| | clearButtonSize | 11.0 | SearchFieldView.swift:35 |
| | clearButtonTrailingPadding | 5.0 | SearchFieldView.swift:36 |
| **Header** | height | 25.0 | HeaderView.swift:31 |
| | horizontalPadding | 10.0 | HeaderView.swift:33 |
| | bottomPadding | 5.0 | HeaderView.swift:36 |
| **Footer** | dividerHorizontalPadding | 10.0 | FooterView.swift:24 |
| | dividerVerticalPadding | 6.0 | FooterView.swift:25 |
| **Separator** | horizontalPadding | 10.0 | HistoryListView.swift:34 |
| | verticalPadding | 3.0 | HistoryListView.swift:35 |

---

## 十二、字体规范

Maccy 使用 **macOS 系统默认字体**（San Francisco），Flutter 对应：

```dart
// 默认字体
static const String fontFamily = '.AppleSystemUIFont';  // macOS
// Windows 对应: 'Segoe UI'

// 字体大小
static const double fontSize = 13.0;  // macOS 系统默认

// 字重
static const FontWeight fontWeightNormal = FontWeight.w400;
static const FontWeight fontWeightMedium = FontWeight.w500;
```

---

## 十三、颜色完整对照表

| 元素 | 状态 | 颜色 | 来源 |
|-----|------|------|------|
| ListItem 背景 | 选中 | `Colors.blue.withOpacity(0.8)` | ListItemView.swift:74 |
| ListItem 背景 | 未选中 | `Colors.white.withOpacity(0.001)` | ListItemView.swift:74 |
| ListItem 文字 | 选中 | `Colors.white` | ListItemView.swift:71 |
| ListItem 文字 | 未选中 | `Colors.black87` (.primary) | ListItemView.swift:71 |
| SearchField 背景 | - | `Colors.grey.withOpacity(0.1)` | SearchFieldView.swift:12 |
| SearchField 图标 | - | `Colors.black54` (opacity: 0.8) | SearchFieldView.swift:20 |
| Header 标题 | - | `Colors.black54` (.secondary) | HeaderView.swift:17 |
| Shortcut 文字 | - | `Colors.black54` (opacity: 0.7) | KeyboardShortcutView.swift:24 |

---

## 十四、实现检查清单

### ✅ 必须实现
- [ ] ListItem 高度固定 22px
- [ ] 无 Pin/Delete 按钮，纯键盘操作
- [ ] 选中背景色 `Colors.blue.withOpacity(0.8)`
- [ ] 未选中背景几乎透明 `withOpacity(0.001)`
- [ ] 圆角 4px
- [ ] 快捷键提示右对齐，宽度 67px
- [ ] 文本中间截断（ellipsis in middle）
- [ ] 搜索框高度 23px，圆角 5px
- [ ] Header/Footer 水平内边距 10px
- [ ] 整体窗口内边距 5px

### ⚠️ 注意事项
- [ ] 所有间距使用 0，条目之间无缝连接
- [ ] Hover 时不改变背景色，只改变选中状态
- [ ] 快捷键符号使用 macOS 标准符号（⌘⇧⌥⌃）
- [ ] 应用图标默认隐藏，需配置开启
- [ ] Pin 分隔线仅在有 pin 且有普通条目时显示

---

## 十五、与您现有代码的对比建议

建议检查您的 Flutter 代码：

1. **移除所有 Pin/Delete 图标按钮**
2. **确保 ListItem 高度固定为 22px**
3. **使用精确的颜色值**（特别是 `opacity: 0.8` 和 `0.001`）
4. **快捷键提示区域宽度 67px**（55 + 1 + 12 - 1）
5. **搜索框高度 23px**（不是 22px）
6. **所有 spacing 设为 0**
7. **圆角统一 4px**（除了搜索框 5px）
8. **文本截断模式改为中间截断**

这样可以确保像素级复刻 Maccy 的原生体验！
