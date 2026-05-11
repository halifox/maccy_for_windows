# Maccy 设置页面完整功能规范

## 概述

Maccy v2.6.1 的设置页面包含 **6 个主要分组**，共 **30+ 个配置项**。本文档按照功能分组和实现优先级提供完整的复刻方案。

---

## 一、设置页面结构

### 1.1 六大设置分组

```
Settings Window
├── General (通用)
├── Storage (存储)
├── Appearance (外观)
├── Pins (固定项)
├── Ignore (忽略规则)
│   ├── Applications (应用程序)
│   ├── Pasteboard Types (剪贴板类型)
│   └── Regular Expressions (正则表达式)
└── Advanced (高级)
```

---

## 二、General (通用设置)

### 2.1 功能条目

#### **启动与更新**
1. **Launch at Login** (开机启动)
   - 类型: Toggle
   - 默认值: false
   - 实现: 使用 `launch_at_startup` 包

2. **Check for Updates** (自动检查更新)
   - 类型: Toggle
   - 默认值: true
   - 实现: 可选功能，Windows 可使用 GitHub Releases API

3. **Check Now** (立即检查更新)
   - 类型: Button
   - 功能: 手动触发更新检查

#### **全局快捷键**
4. **Open** (打开 Maccy)
   - 类型: Keyboard Shortcut Recorder
   - 默认值: `Cmd+Shift+C` (Windows: `Ctrl+Shift+C`)
   - 配置键: 无 (使用 `hotkey_manager`)
   - 提示: "Press a new shortcut to record it"

5. **Pin** (固定条目)
   - 类型: Keyboard Shortcut Recorder
   - 默认值: `Cmd+P` (Windows: `Ctrl+P`)
   - 提示: "Pin the selected item"

6. **Delete** (删除条目)
   - 类型: Keyboard Shortcut Recorder
   - 默认值: `Cmd+Delete` (Windows: `Ctrl+Delete`)
   - 提示: "Delete the selected item"

#### **搜索模式**
7. **Search** (搜索模式)
   - 类型: Dropdown (Picker)
   - 选项:
     - `Exact` (精确匹配) - 默认
     - `Fuzzy` (模糊搜索)
     - `Regex` (正则表达式)
     - `Mixed` (混合模式: Exact → Regex → Fuzzy)
   - 配置键: `searchMode`
   - 默认值: `exact`

#### **行为设置**
8. **Paste Automatically** (自动粘贴)
   - 类型: Toggle
   - 配置键: `pasteByDefault`
   - 默认值: false
   - 说明: 选择条目后自动执行粘贴

9. **Paste Without Formatting** (默认去除格式)
   - 类型: Toggle
   - 配置键: `removeFormattingByDefault`
   - 默认值: false
   - 说明: 粘贴时默认去除富文本格式

10. **Modifiers** (修饰键说明)
    - 类型: 只读文本
    - 内容: 动态显示当前修饰键组合
    - 格式: "⌘ to copy, ⌥⌘ to paste, ⇧⌘ to paste without formatting"

#### **通知设置**
11. **Notifications and Sounds** (通知与声音)
    - 类型: Link
    - 功能: 打开系统通知设置
    - Windows: 打开 Windows 通知设置

---

## 三、Storage (存储设置)

### 3.1 功能条目

#### **保存类型**
1. **Save Files** (保存文件)
   - 类型: Toggle
   - 配置键: `enabledPasteboardTypes` (包含 `.fileURL`)
   - 默认值: true
   - 说明: 保存复制的文件路径

2. **Save Images** (保存图片)
   - 类型: Toggle
   - 配置键: `enabledPasteboardTypes` (包含 `.png`, `.tiff`)
   - 默认值: true
   - 说明: 保存复制的图片

3. **Save Text** (保存文本)
   - 类型: Toggle
   - 配置键: `enabledPasteboardTypes` (包含 `.html`, `.rtf`, `.string`)
   - 默认值: true
   - 说明: 保存复制的文本内容

4. **Description** (说明文本)
   - 内容: "Maccy stores the clipboard history in a local database."

#### **历史记录大小**
5. **Size** (历史记录数量)
   - 类型: TextField + Stepper
   - 配置键: `size` (或 `historySize`)
   - 默认值: 200
   - 范围: 1 - 999
   - 显示: 当前数据库大小 (如 "2.3 MB")

#### **排序方式**
6. **Sort By** (排序依据)
   - 类型: Dropdown (Picker)
   - 配置键: `sortBy`
   - 选项:
     - `Last Copied At` (最后复制时间) - 默认
     - `First Copied At` (首次复制时间)
     - `Number of Copies` (复制次数)
   - 默认值: `lastCopiedAt`

---

## 四、Appearance (外观设置)

### 4.1 功能条目

#### **弹窗位置**
1. **Popup At** (弹出位置)
   - 类型: Dropdown (Picker)
   - 配置键: `popupPosition`
   - 选项:
     - `Cursor` (鼠标位置) - 默认
     - `Center` (屏幕中心)
     - `Status Item` (状态栏图标下方)
     - `Last Position` (上次位置)
   - 默认值: `cursor`
   - 多屏支持: Center 和 Last Position 可选择屏幕

2. **Reset Last Position** (重置上次位置)
   - 类型: Button (仅在选择 Last Position 时显示)
   - 功能: 重置 `windowPosition` 为默认值

#### **固定项位置**
3. **Pin To** (固定项位置)
   - 类型: Dropdown (Picker)
   - 配置键: `pinTo`
   - 选项:
     - `Top` (顶部) - 默认
     - `Bottom` (底部)
   - 默认值: `top`

#### **图片设置**
4. **Image Height** (图片高度)
   - 类型: TextField + Stepper
   - 配置键: `imageMaxHeight`
   - 默认值: 40
   - 范围: 1 - 200
   - 单位: pixels

#### **预览设置**
5. **Preview Delay** (预览延迟)
   - 类型: TextField + Stepper
   - 配置键: `previewDelay`
   - 默认值: 1500
   - 范围: 200 - 100,000
   - 单位: milliseconds

#### **搜索高亮**
6. **Highlight Matches** (搜索高亮方式)
   - 类型: Dropdown (Picker)
   - 配置键: `highlightMatch`
   - 选项:
     - `Bold` (粗体) - 默认
     - `Italic` (斜体)
     - `Underline` (下划线)
   - 默认值: `bold`

#### **显示选项**
7. **Show Special Symbols** (显示特殊符号)
   - 类型: Toggle
   - 配置键: `showSpecialSymbols`
   - 默认值: true
   - 说明: 将空格显示为 `·`，换行显示为 `⏎`，Tab 显示为 `⇥`

8. **Show Menu Icon** (显示菜单图标)
   - 类型: Toggle + Icon Picker
   - 配置键: `showInStatusBar`
   - 默认值: true
   - 图标选项:
     - `Maccy` (默认图标)
     - `Clipboard` (剪贴板图标)
     - `Scissors` (剪刀图标)
   - 配置键: `menuIcon`

9. **Show Recent Copy in Menu Bar** (状态栏显示最近复制)
   - 类型: Toggle
   - 配置键: `showRecentCopyInMenuBar`
   - 默认值: false
   - 说明: 在状态栏图标旁显示最近复制的内容

10. **Show Search Field** (显示搜索框)
    - 类型: Toggle + Visibility Picker
    - 配置键: `showSearch`
    - 默认值: true
    - 可见性选项:
      - `Always` (始终显示) - 默认
      - `During Search` (搜索时显示)
    - 配置键: `searchVisibility`

11. **Show Title Before Search Field** (显示标题)
    - 类型: Toggle
    - 配置键: `showTitle`
    - 默认值: true
    - 说明: 在搜索框前显示 "Maccy" 标题

12. **Show Application Icons** (显示应用图标)
    - 类型: Toggle
    - 配置键: `showApplicationIcons`
    - 默认值: false
    - 说明: 在条目前显示来源应用的图标

13. **Show Footer** (显示底部栏)
    - 类型: Toggle
    - 配置键: `showFooter`
    - 默认值: true
    - 警告: 隐藏后只能通过 `Cmd+,` 打开设置

---

## 五、Pins (固定项管理)

### 5.1 功能条目

#### **固定项列表**
1. **Pins Table** (固定项表格)
   - 类型: Table (3 列)
   - 列定义:
     - **Key** (快捷键): Dropdown, 宽度 60px
       - 可选字符: `b-y` (排除 `a/q/v/w/z`)
       - 可编辑
     - **Alias** (别名): TextField
       - 可编辑条目标题
     - **Content** (内容): TextField
       - 可编辑纯文本内容
       - 富文本显示警告图标
       - 非文本内容显示 "Content is not text"

2. **Delete Command** (删除快捷键)
   - 功能: 按 `Delete` 键删除选中的固定项

3. **Description** (说明文本)
   - 内容: "You can customize the key, alias, and content of pinned items."

#### **实现逻辑**
- 查询条件: `pin != null`
- 排序: `firstCopiedAt`
- 编辑内容时:
  - 移除所有非纯文本格式
  - 仅保留 `NSPasteboard.PasteboardType.string`
  - 更新 `HistoryItemContent.value`

---

## 六、Ignore (忽略规则)

### 6.1 结构

使用 **TabView** 包含 3 个子页面：

#### **Tab 1: Applications (应用程序)**

1. **Ignored Apps List** (忽略应用列表)
   - 类型: List
   - 显示: 应用图标 + 应用名称
   - 数据源: `ignoredApps` (Bundle ID 数组)
   - 默认值: `[]`

2. **Add Button** (+)
   - 功能: 打开文件选择器，选择 `.app` 文件
   - 默认目录: `/Applications` (Windows: `C:\Program Files`)
   - 提取: Bundle Identifier 并添加到列表

3. **Remove Button** (-)
   - 功能: 删除选中的应用

4. **Ignore All Apps Except Listed** (反向过滤)
   - 类型: Toggle
   - 配置键: `ignoreAllAppsExceptListed`
   - 默认值: false
   - 说明: 启用后，仅监听列表中的应用

5. **Description** (说明文本)
   - 内容: "Maccy will ignore clipboard changes from these applications."

#### **Tab 2: Pasteboard Types (剪贴板类型)**

1. **Ignored Types List** (忽略类型列表)
   - 类型: List (可编辑)
   - 数据源: `ignoredPasteboardTypes` (Set<String>)
   - 默认值:
     ```
     "Pasteboard generator type"
     "com.agilebits.onepassword"
     "com.typeit4me.clipping"
     "de.petermaurer.TransientPasteboardType"
     "net.antelle.keeweb"
     ```

2. **Add Button** (+)
   - 功能: 添加新类型 `"xxx.yyy.zzz"`
   - 自动聚焦到新添加的输入框

3. **Remove Button** (-)
   - 功能: 删除选中的类型

4. **Reset Button** (重置)
   - 功能: 恢复默认的忽略类型列表

5. **Description** (说明文本)
   - 内容: "Maccy will ignore clipboard changes with these pasteboard types."

#### **Tab 3: Regular Expressions (正则表达式)**

1. **Ignored Regexps List** (忽略正则列表)
   - 类型: List (可编辑)
   - 数据源: `ignoreRegexp` (Array<String>)
   - 默认值: `[]`

2. **Add Button** (+)
   - 功能: 添加新正则 `"^[a-zA-Z0-9]{50}$"`
   - 自动聚焦到新添加的输入框

3. **Remove Button** (-)
   - 功能: 删除选中的正则

4. **Description** (说明文本)
   - 内容: "Maccy will ignore clipboard changes matching these regular expressions."

---

## 七、Advanced (高级设置)

### 7.1 功能条目

#### **暂停监听**
1. **Turn Off** (关闭 Maccy)
   - 类型: Toggle
   - 配置键: `ignoreEvents`
   - 默认值: false
   - 说明: 暂停剪贴板监听

2. **Description** (说明文本)
   - 内容: "Maccy will not save any clipboard changes while turned off."

3. **Shell Script Example** (Shell 脚本示例)
   - 内容:
     ```bash
     # Turn off
     defaults write org.p0deje.Maccy ignoreEvents true
     # Turn on
     defaults write org.p0deje.Maccy ignoreEvents false
     ```

4. **Menu Icon Description** (菜单图标说明)
   - 内容: "You can also turn off Maccy by Option-clicking the menu icon."

5. **Next Event Shell Script** (仅忽略下一次)
   - 内容:
     ```bash
     defaults write org.p0deje.Maccy ignoreOnlyNextEvent true
     ```

#### **退出时清空**
6. **Clear History on Quit** (退出时清空历史)
   - 类型: Toggle
   - 配置键: `clearOnQuit`
   - 默认值: false
   - 说明: 退出应用时自动清空历史记录

7. **Clear System Clipboard** (同时清空系统剪贴板)
   - 类型: Toggle
   - 配置键: `clearSystemClipboard`
   - 默认值: false
   - 说明: 清空历史时同时清空系统剪贴板

---

## 八、配置键完整列表

### 8.1 所有配置项

| 配置键 | 类型 | 默认值 | 说明 |
|-------|------|--------|------|
| `clearOnQuit` | Bool | false | 退出时清空历史 |
| `clearSystemClipboard` | Bool | false | 清空时同时清空系统剪贴板 |
| `clipboardCheckInterval` | Double | 0.5 | 剪贴板检查间隔（秒） |
| `enabledPasteboardTypes` | Set<String> | all | 启用的剪贴板类型 |
| `highlightMatch` | String | "bold" | 搜索高亮方式 |
| `ignoreAllAppsExceptListed` | Bool | false | 反向过滤模式 |
| `ignoreEvents` | Bool | false | 暂停监听 |
| `ignoreOnlyNextEvent` | Bool | false | 仅忽略下一次 |
| `ignoreRegexp` | Array<String> | [] | 忽略的正则表达式 |
| `ignoredApps` | Array<String> | [] | 忽略的应用 Bundle ID |
| `ignoredPasteboardTypes` | Set<String> | 见上文 | 忽略的剪贴板类型 |
| `imageMaxHeight` | Int | 40 | 图片最大高度 |
| `menuIcon` | String | "maccy" | 菜单图标样式 |
| `pasteByDefault` | Bool | false | 自动粘贴 |
| `pinTo` | String | "top" | 固定项位置 |
| `popupPosition` | String | "cursor" | 弹出位置 |
| `popupScreen` | Int | 0 | 弹出屏幕索引 |
| `previewDelay` | Int | 1500 | 预览延迟（毫秒） |
| `removeFormattingByDefault` | Bool | false | 默认去除格式 |
| `searchMode` | String | "exact" | 搜索模式 |
| `showFooter` | Bool | true | 显示底部栏 |
| `showInStatusBar` | Bool | true | 显示状态栏图标 |
| `showRecentCopyInMenuBar` | Bool | false | 状态栏显示最近复制 |
| `showSearch` | Bool | true | 显示搜索框 |
| `searchVisibility` | String | "always" | 搜索框可见性 |
| `showSpecialSymbols` | Bool | true | 显示特殊符号 |
| `showTitle` | Bool | true | 显示标题 |
| `size` | Int | 200 | 历史记录数量 |
| `sortBy` | String | "lastCopiedAt" | 排序方式 |
| `suppressClearAlert` | Bool | false | 禁用清空确认对话框 |
| `windowSize` | Size | 450x800 | 窗口尺寸 |
| `windowPosition` | Point | 0.5, 0.8 | 窗口位置 |
| `showApplicationIcons` | Bool | false | 显示应用图标 |

---

## 九、实现优先级

### P0 (核心功能)
1. ✅ General - 全局快捷键 (Open, Pin, Delete)
2. ✅ General - 搜索模式 (Exact, Fuzzy, Regex, Mixed)
3. ✅ General - 行为设置 (自动粘贴、去除格式)
4. ✅ Storage - 保存类型 (Files, Images, Text)
5. ✅ Storage - 历史记录大小
6. ✅ Storage - 排序方式
7. ✅ Appearance - 弹出位置
8. ✅ Appearance - 固定项位置
9. ✅ Appearance - 显示选项 (搜索框、底部栏、标题)

### P1 (重要功能)
10. ⚠️ General - 开机启动
11. ⚠️ Appearance - 图片高度
12. ⚠️ Appearance - 预览延迟
13. ⚠️ Appearance - 搜索高亮
14. ⚠️ Appearance - 特殊符号显示
15. ⚠️ Appearance - 应用图标显示
16. ⚠️ Ignore - 应用程序过滤
17. ⚠️ Ignore - 剪贴板类型过滤
18. ⚠️ Ignore - 正则表达式过滤
19. ⚠️ Advanced - 暂停监听
20. ⚠️ Advanced - 退出时清空

### P2 (增强功能)
21. ❌ General - 自动更新检查
22. ❌ Appearance - 菜单图标样式
23. ❌ Appearance - 状态栏显示最近复制
24. ❌ Appearance - 多屏支持
25. ❌ Pins - 固定项管理表格
26. ❌ Advanced - Shell 脚本集成

---

## 十、Flutter 实现建议

### 10.1 数据持久化

使用 `shared_preferences` 存储所有配置：

```dart
class SettingsService {
  final SharedPreferences _prefs;
  
  // General
  Future<void> setSearchMode(String mode) async {
    await _prefs.setString('searchMode', mode);
  }
  
  Future<bool> getPasteByDefault() async {
    return _prefs.getBool('pasteByDefault') ?? false;
  }
  
  // Storage
  Future<void> setHistorySize(int size) async {
    await _prefs.setInt('size', size);
  }
  
  Future<void> setEnabledTypes(Set<String> types) async {
    await _prefs.setStringList('enabledPasteboardTypes', types.toList());
  }
  
  // Appearance
  Future<void> setPopupPosition(String position) async {
    await _prefs.setString('popupPosition', position);
  }
  
  // Ignore
  Future<void> setIgnoredApps(List<String> apps) async {
    await _prefs.setStringList('ignoredApps', apps);
  }
  
  Future<void> setIgnoreRegexp(List<String> regexps) async {
    await _prefs.setStringList('ignoreRegexp', regexps);
  }
}
```

### 10.2 UI 组件结构

```dart
// 主设置页面
class SettingsPage extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: 6,
      child: Scaffold(
        appBar: AppBar(
          title: Text('Settings'),
          bottom: TabBar(
            tabs: [
              Tab(text: 'General'),
              Tab(text: 'Storage'),
              Tab(text: 'Appearance'),
              Tab(text: 'Pins'),
              Tab(text: 'Ignore'),
              Tab(text: 'Advanced'),
            ],
          ),
        ),
        body: TabBarView(
          children: [
            GeneralSettingsPane(),
            StorageSettingsPane(),
            AppearanceSettingsPane(),
            PinsSettingsPane(),
            IgnoreSettingsPane(),
            AdvancedSettingsPane(),
          ],
        ),
      ),
    );
  }
}
```

### 10.3 快捷键录制器

```dart
class HotkeyRecorder extends StatefulWidget {
  final String label;
  final String? currentHotkey;
  final ValueChanged<String?> onChanged;
  
  @override
  Widget build(BuildContext context) {
    return Row(
      children: [
        Text(label),
        Spacer(),
        GestureDetector(
          onTap: () => _startRecording(),
          child: Container(
            padding: EdgeInsets.symmetric(horizontal: 12, vertical: 6),
            decoration: BoxDecoration(
              border: Border.all(color: Colors.grey),
              borderRadius: BorderRadius.circular(4),
            ),
            child: Text(
              currentHotkey ?? 'Click to record',
              style: TextStyle(fontFamily: 'monospace'),
            ),
          ),
        ),
      ],
    );
  }
}
```

---

## 十一、Windows 平台适配

### 11.1 快捷键映射

| macOS | Windows |
|-------|---------|
| `⌘` (Command) | `Ctrl` |
| `⌥` (Option) | `Alt` |
| `⇧` (Shift) | `Shift` |
| `⌃` (Control) | `Win` |
| `⌫` (Delete) | `Backspace` |

### 11.2 应用识别

- macOS: Bundle Identifier (`com.apple.Safari`)
- Windows: 进程名 (`chrome.exe`) 或可执行文件路径

### 11.3 剪贴板类型

- macOS: `NSPasteboard.PasteboardType`
- Windows: Clipboard Format (CF_TEXT, CF_UNICODETEXT, CF_BITMAP, CF_HDROP)

---

## 十二、总结

Maccy 的设置页面功能完整且逻辑清晰，共包含：

- **6 个主要分组**
- **30+ 个配置项**
- **3 个子 Tab 页面** (Ignore)
- **1 个表格编辑器** (Pins)

建议按照 P0 → P1 → P2 的优先级逐步实现，确保核心功能稳定后再添加增强特性。所有配置项都应使用 `shared_preferences` 持久化，并通过 Riverpod 进行状态管理。
