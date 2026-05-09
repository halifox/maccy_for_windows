# Maccy Windows 移植完整指南

基于 Maccy 2.6.1 源码深度分析，本文档提供功能对齐清单、架构设计和实现方案。

---

## 一、功能对齐清单（Feature Parity Checklist）

### 1.1 核心功能模块

| 功能模块 | Maccy 实现 | Flutter 实现状态 | 优先级 |
|---------|-----------|----------------|--------|
| **剪贴板监听** | NSPasteboard + Timer (0.5s) | clipboard_watcher | ✅ P0 |
| **历史记录存储** | SwiftData (SQLite) | Drift | ✅ P0 |
| **弹窗显示/隐藏** | NSPanel + GlobalHotKey | window_manager + hotkey_manager | ✅ P0 |
| **搜索功能** | Exact/Fuzzy/Regexp/Mixed | fuzzy + RegExp | ✅ P0 |
| **固定项管理** | Pin with shortcuts (b-y) | 需实现 | 🔶 P1 |
| **排序策略** | lastCopied/firstCopied/copies | 需实现 | 🔶 P1 |
| **快捷键系统** | 数字1-10 + 字母b-y | 需实现 | 🔶 P1 |
| **多种粘贴模式** | Copy/Paste/PasteWithoutFormat | 需实现 | 🔶 P1 |
| **应用过滤** | 黑名单/白名单 | 需实现 | 🔶 P1 |
| **正则过滤** | ignoreRegexp | 需实现 | 🔶 P1 |
| **图片 OCR** | Vision Framework | google_mlkit_text_recognition | 🔷 P2 |
| **多屏支持** | NSScreen API | screen_retriever | ✅ P0 |
| **系统托盘** | NSStatusItem | tray_manager | ✅ P0 |

### 1.2 配置项对齐（Settings Parity）

#### General 设置
- [x] `launchAtStartup` - 开机启动
- [x] `autoCheckUpdates` - 自动更新检查
- [x] `hotkeyOpen` - 全局唤起快捷键
- [ ] `hotkeyPin` - 固定项快捷键（需实现）
- [ ] `hotkeyDelete` - 删除项快捷键（需实现）
- [x] `searchMode` - 搜索模式（exact/fuzzy/regexp/mixed）
- [x] `autoPaste` - 自动粘贴
- [x] `pastePlain` - 纯文本粘贴

#### Storage 设置
- [x] `historyLimit` - 历史记录上限（200）
- [x] `sortBy` - 排序方式
- [x] `saveText/saveImages/saveFiles` - 类型过滤
- [x] `clearOnExit` - 退出时清空
- [x] `clearSystemClipboard` - 清空系统剪贴板

#### Appearance 设置
- [x] `popupPosition` - 弹窗位置（cursor/center/statusItem/lastPosition）
- [x] `popupScreen` - 目标屏幕
- [x] `pinPosition` - 固定项位置（top/bottom）
- [x] `imageHeight` - 图片高度
- [x] `previewDelay` - 预览延迟
- [x] `highlightMatch` - 高亮样式
- [x] `showSpecialChars` - 显示特殊字符
- [x] `showMenuBarIcon` - 显示托盘图标
- [x] `menuBarIconType` - 图标类型
- [x] `showRecentCopyInMenuBar` - 显示最近复制
- [x] `showSearch` - 显示搜索框
- [x] `searchVisibility` - 搜索框可见性
- [x] `showTitle` - 显示标题
- [x] `showAppIcon` - 显示应用图标
- [x] `showFooterMenu` - 显示底部菜单
- [x] `windowWidth/windowHeight` - 窗口尺寸
- [x] `themeMode` - 主题模式

#### Ignore 设置
- [x] `ignoredApps` - 忽略应用列表
- [x] `ignoreAllAppsExceptListed` - 白名单模式
- [x] `ignoredPasteboardTypes` - 忽略类型
- [x] `ignoreRegexp` - 正则过滤

#### Advanced 设置
- [x] `clipboardCheckInterval` - 检查间隔
- [x] `ignoreEvents` - 暂停监听
- [x] `ignoreOnlyNextEvent` - 仅忽略下一次

---

## 二、架构设计方案

### 2.1 数据模型（Data Models）

#### HistoryItem 核心字段映射

```dart
// lib/core/database/database.dart
@DataClassName('HistoryItem')
class HistoryItems extends Table {
  IntColumn get id => integer().autoIncrement()();
  
  // 基础信息
  TextColumn get application => text().nullable()();  // 来源应用
  DateTimeColumn get firstCopiedAt => dateTime()();   // 首次复制时间
  DateTimeColumn get lastCopiedAt => dateTime()();    // 最后复制时间
  IntColumn get numberOfCopies => integer().withDefault(const Constant(1))();
  
  // 固定项
  TextColumn get pin => text().nullable()();  // 快捷键 (b-y)
  
  // 显示标题
  TextColumn get title => text()();
  
  // 内容类型标记
  BoolColumn get hasText => boolean().withDefault(const Constant(false))();
  BoolColumn get hasRtf => boolean().withDefault(const Constant(false))();
  BoolColumn get hasHtml => boolean().withDefault(const Constant(false))();
  BoolColumn get hasImage => boolean().withDefault(const Constant(false))();
  BoolColumn get hasFiles => boolean().withDefault(const Constant(false))();
}

@DataClassName('HistoryItemContent')
class HistoryItemContents extends Table {
  IntColumn get id => integer().autoIncrement()();
  IntColumn get itemId => integer().references(HistoryItems, #id, onDelete: KeyAction.cascade)();
  
  TextColumn get type => text()();      // 'text', 'rtf', 'html', 'image', 'files'
  BlobColumn get value => blob()();     // 二进制数据
}
```

### 2.2 核心服务层（Core Services）

#### ClipboardManager - 剪贴板监听

```dart
// lib/core/managers/clipboard_manager_provider.dart
@riverpod
class ClipboardManager extends _$ClipboardManager {
  Timer? _timer;
  int _changeCount = 0;
  
  @override
  FutureOr<void> build() async {
    ref.onDispose(() => _timer?.cancel());
    await _startMonitoring();
  }
  
  Future<void> _startMonitoring() async {
    final interval = ref.watch(clipboardCheckIntervalProvider);
    _timer = Timer.periodic(
      Duration(milliseconds: (interval * 1000).toInt()),
      (_) => _checkClipboard(),
    );
  }
  
  Future<void> _checkClipboard() async {
    // 1. 检测 changeCount
    final currentCount = await _getChangeCount();
    if (currentCount == _changeCount) return;
    _changeCount = currentCount;
    
    // 2. 读取剪贴板内容
    final clipboard = SystemClipboard.instance;
    final reader = await clipboard?.read();
    if (reader == null) return;
    
    // 3. 过滤检查
    if (_shouldIgnore(reader)) return;
    
    // 4. 创建 HistoryItem
    final item = await _createHistoryItem(reader);
    
    // 5. 去重和存储
    await ref.read(historyRepositoryProvider).addOrUpdate(item);
  }
  
  bool _shouldIgnore(DataReader reader) {
    // 实现 Maccy 的过滤逻辑：
    // - ignoredApps 检查
    // - ignoredPasteboardTypes 检查
    // - ignoreRegexp 检查
    // - ignoreEvents 检查
    return false;
  }
}
```

#### SearchService - 搜索引擎

```dart
// lib/core/services/search_service.dart
class SearchService {
  List<SearchResult> search(
    String query,
    List<HistoryItem> items,
    SearchMode mode,
  ) {
    switch (mode) {
      case SearchMode.exact:
        return _exactSearch(query, items);
      case SearchMode.fuzzy:
        return _fuzzySearch(query, items);
      case SearchMode.regexp:
        return _regexpSearch(query, items);
      case SearchMode.mixed:
        return _mixedSearch(query, items);
    }
  }
  
  List<SearchResult> _exactSearch(String query, List<HistoryItem> items) {
    final lowerQuery = query.toLowerCase();
    return items
        .where((item) => item.title.toLowerCase().contains(lowerQuery))
        .map((item) => SearchResult(
              item: item,
              score: 1.0,
              ranges: _findMatchRanges(item.title, query),
            ))
        .toList();
  }
  
  List<SearchResult> _fuzzySearch(String query, List<HistoryItem> items) {
    // 使用 fuzzy 包实现模糊搜索
    final fuzzy = Fuzzy<HistoryItem>(
      items,
      options: FuzzyOptions(
        keys: [WeightedKey(getter: (item) => item.title, weight: 1.0)],
        threshold: 0.3, // Maccy 使用 0.7，但 fuzzy 包的阈值是反向的
      ),
    );
    
    return fuzzy.search(query).map((result) {
      return SearchResult(
        item: result.item,
        score: result.score,
        ranges: result.matches.first.matchedIndices,
      );
    }).toList();
  }
  
  List<SearchResult> _regexpSearch(String query, List<HistoryItem> items) {
    try {
      final regex = RegExp(query, caseSensitive: false);
      return items
          .where((item) => regex.hasMatch(item.title))
          .map((item) => SearchResult(
                item: item,
                score: 1.0,
                ranges: _findRegexRanges(item.title, regex),
              ))
          .toList();
    } catch (_) {
      return [];
    }
  }
  
  List<SearchResult> _mixedSearch(String query, List<HistoryItem> items) {
    // 依次尝试 exact → regexp → fuzzy
    var results = _exactSearch(query, items);
    if (results.isNotEmpty) return results;
    
    results = _regexpSearch(query, items);
    if (results.isNotEmpty) return results;
    
    return _fuzzySearch(query, items);
  }
}
```

#### PasteService - 粘贴服务

```dart
// lib/core/services/paste_service.dart
enum PasteMode {
  copy,                    // 仅复制
  paste,                   // 复制并粘贴
  pasteWithoutFormatting,  // 去格式粘贴
}

class PasteService {
  Future<void> execute(HistoryItem item, PasteMode mode) async {
    // 1. 写入剪贴板
    await _writeToClipboard(item, removeFormatting: mode == PasteMode.pasteWithoutFormatting);
    
    // 2. 如果需要粘贴，模拟 Ctrl+V
    if (mode == PasteMode.paste || mode == PasteMode.pasteWithoutFormatting) {
      await _simulatePaste();
    }
  }
  
  Future<void> _writeToClipboard(HistoryItem item, {bool removeFormatting = false}) async {
    final clipboard = SystemClipboard.instance;
    if (clipboard == null) return;
    
    final writer = ClipboardWriter();
    
    if (removeFormatting) {
      // 仅写入纯文本
      if (item.hasText) {
        writer.write(Formats.plainText(item.text!));
      }
    } else {
      // 写入所有格式
      if (item.hasText) writer.write(Formats.plainText(item.text!));
      if (item.hasRtf) writer.write(Formats.rtf(item.rtfData!));
      if (item.hasHtml) writer.write(Formats.html(item.htmlData!));
      if (item.hasImage) writer.write(Formats.png(item.imageData!));
      if (item.hasFiles) {
        for (final path in item.filePaths!) {
          writer.write(Formats.fileUri(Uri.file(path)));
        }
      }
    }
    
    await clipboard.write([writer]);
  }
  
  Future<void> _simulatePaste() async {
    // Windows: 模拟 Ctrl+V
    // 使用 win32 API 或 ffi 调用 SendInput
    await Future.delayed(const Duration(milliseconds: 100));
    // TODO: 实现键盘事件模拟
  }
}
```

### 2.3 UI 层架构

#### HistoryPage - 主界面

```dart
// lib/features/history/ui/history_page.dart
class HistoryPage extends HookConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    final searchQuery = useState('');
    final selectedIndex = useState(0);
    final items = ref.watch(filteredHistoryProvider(searchQuery.value));
    
    return Scaffold(
      backgroundColor: Colors.transparent,
      body: Column(
        children: [
          // 顶部搜索栏
          if (ref.watch(showSearchProvider))
            _SearchBar(
              query: searchQuery,
              onChanged: (value) => searchQuery.value = value,
            ),
          
          // 历史列表
          Expanded(
            child: _HistoryList(
              items: items,
              selectedIndex: selectedIndex.value,
              onSelect: (index) => selectedIndex.value = index,
              onActivate: (item) => _handleActivate(ref, item),
            ),
          ),
          
          // 底部操作栏
          if (ref.watch(showFooterMenuProvider))
            _FooterBar(),
        ],
      ),
    );
  }
  
  void _handleActivate(WidgetRef ref, HistoryItem item) {
    final mode = _determinePasteMode(ref);
    ref.read(pasteServiceProvider).execute(item, mode);
    ref.read(windowManagerProvider.notifier).hide();
  }
  
  PasteMode _determinePasteMode(WidgetRef ref) {
    // 根据修饰键和配置决定粘贴模式
    final modifiers = ref.read(modifierKeyServiceProvider).current;
    final autoPaste = ref.read(autoPasteProvider);
    final pastePlain = ref.read(pastePlainProvider);
    
    if (modifiers.contains(ModifierKey.shift)) {
      return PasteMode.pasteWithoutFormatting;
    }
    if (autoPaste) {
      return pastePlain ? PasteMode.pasteWithoutFormatting : PasteMode.paste;
    }
    return PasteMode.copy;
  }
}
```

---

## 三、关键实现方案

### 3.1 固定项系统（Pin System）

```dart
// lib/features/history/services/pin_service.dart
class PinService {
  static const availableKeys = [
    'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'r', 's', 't', 'u', 'x', 'y'
  ]; // 排除 a, q, v, w, z
  
  Future<void> togglePin(HistoryItem item) async {
    if (item.pin != null) {
      // 取消固定
      await _unpin(item);
    } else {
      // 分配快捷键并固定
      final key = await _findAvailableKey();
      if (key != null) {
        await _pin(item, key);
      }
    }
  }
  
  Future<String?> _findAvailableKey() async {
    final pinnedItems = await _repository.getPinnedItems();
    final usedKeys = pinnedItems.map((item) => item.pin).toSet();
    
    for (final key in availableKeys) {
      if (!usedKeys.contains(key)) return key;
    }
    return null;
  }
}
```

### 3.2 快捷键系统（Keyboard Shortcuts）

```dart
// lib/core/services/shortcut_service.dart
class ShortcutService {
  void registerItemShortcuts(List<HistoryItem> items) {
    // 数字快捷键 1-10（未固定项）
    final unpinned = items.where((item) => item.pin == null).take(10).toList();
    for (var i = 0; i < unpinned.length; i++) {
      _registerShortcut('${i + 1}', () => _selectItem(unpinned[i]));
    }
    
    // 字母快捷键（固定项）
    final pinned = items.where((item) => item.pin != null).toList();
    for (final item in pinned) {
      _registerShortcut(item.pin!, () => _selectItem(item));
    }
  }
  
  void _registerShortcut(String key, VoidCallback callback) {
    // 使用 RawKeyboardListener 或 Focus 节点监听
  }
}
```

### 3.3 弹窗定位系统（Popup Positioning）

```dart
// lib/core/services/window_position_service.dart
class WindowPositionService {
  Future<Offset> calculatePosition(PopupPosition mode) async {
    switch (mode) {
      case PopupPosition.cursor:
        return await _getCursorPosition();
      
      case PopupPosition.center:
        final screen = await _getTargetScreen();
        return Offset(
          screen.visibleFrame.center.dx - windowWidth / 2,
          screen.visibleFrame.center.dy - windowHeight / 2,
        );
      
      case PopupPosition.statusItem:
        // Windows: 任务栏托盘图标位置
        return await _getTrayIconPosition();
      
      case PopupPosition.lastPosition:
        return _getLastPosition();
    }
  }
  
  Future<Offset> _getCursorPosition() async {
    // 使用 win32 API 获取鼠标位置
    // GetCursorPos()
    return Offset.zero;
  }
}
```

### 3.4 应用过滤系统（App Filtering）

```dart
// lib/core/services/clipboard_filter_service.dart
class ClipboardFilterService {
  bool shouldIgnore(String? appIdentifier) {
    final ignoredApps = ref.read(ignoredAppsProvider);
    final whitelistMode = ref.read(ignoreAllAppsExceptListedProvider);
    
    if (appIdentifier == null) return false;
    
    if (whitelistMode) {
      // 白名单模式：仅记录列表内的应用
      return !ignoredApps.contains(appIdentifier);
    } else {
      // 黑名单模式：忽略列表内的应用
      return ignoredApps.contains(appIdentifier);
    }
  }
  
  bool matchesRegexp(String text) {
    final patterns = ref.read(ignoreRegexpProvider);
    for (final pattern in patterns) {
      try {
        if (RegExp(pattern).hasMatch(text)) return true;
      } catch (_) {
        continue;
      }
    }
    return false;
  }
}
```

---

## 四、实现优先级和路线图

### Phase 1: 核心功能完善（P0 - 1周）
- [x] 剪贴板监听和存储
- [x] 基础 UI 和搜索
- [x] 全局快捷键
- [ ] 多种粘贴模式实现
- [ ] 应用过滤系统

### Phase 2: 高级功能（P1 - 2周）
- [ ] 固定项系统
- [ ] 项目快捷键（1-10, b-y）
- [ ] 排序策略
- [ ] 正则过滤
- [ ] 预览弹窗
- [ ] 高亮匹配

### Phase 3: 视觉优化（P1 - 1周）
- [ ] 毛玻璃效果（Acrylic/Mica）
- [ ] 动画过渡
- [ ] 图标和缩略图
- [ ] 特殊字符显示

### Phase 4: 扩展功能（P2 - 按需）
- [ ] 图片 OCR
- [ ] 多语言支持
- [ ] 自动更新
- [ ] 统计和分析

---

## 五、关键技术难点和解决方案

### 5.1 Windows 剪贴板监听

**问题**: Windows 剪贴板没有原生的 changeCount API

**解决方案**:
```dart
// 使用 clipboard_watcher 包
ClipboardWatcher.instance.addListener(() async {
  final data = await Clipboard.getData(Clipboard.kTextPlain);
  if (data?.text != _lastText) {
    _lastText = data?.text;
    _handleClipboardChange();
  }
});
```

### 5.2 全局快捷键冲突

**问题**: Windows 系统快捷键可能冲突

**解决方案**:
- 默认使用 `Alt+V` 而非 `Cmd+Shift+C`
- 提供快捷键冲突检测
- 允许用户自定义

### 5.3 富文本格式保留

**问题**: super_clipboard 对 RTF 支持有限

**解决方案**:
```dart
// 使用 win32 API 直接操作剪贴板
import 'package:win32/win32.dart';

void writeRtfToClipboard(Uint8List rtfData) {
  final hMem = GlobalAlloc(GMEM_MOVEABLE, rtfData.length);
  final pMem = GlobalLock(hMem);
  // 写入 RTF 数据
  GlobalUnlock(hMem);
  SetClipboardData(CF_RTF, hMem);
}
```

### 5.4 模拟键盘粘贴

**问题**: Flutter 无法直接模拟系统级键盘事件

**解决方案**:
```dart
import 'dart:ffi';
import 'package:win32/win32.dart';

void simulateCtrlV() {
  // 按下 Ctrl
  keybd_event(VK_CONTROL, 0, 0, 0);
  // 按下 V
  keybd_event(0x56, 0, 0, 0);
  // 释放 V
  keybd_event(0x56, 0, KEYEVENTF_KEYUP, 0);
  // 释放 Ctrl
  keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
}
```

### 5.5 获取前台应用信息

**问题**: 需要识别复制来源应用

**解决方案**:
```dart
import 'package:win32/win32.dart';

String? getForegroundAppName() {
  final hwnd = GetForegroundWindow();
  if (hwnd == 0) return null;
  
  final processId = calloc<DWORD>();
  GetWindowThreadProcessId(hwnd, processId);
  
  final hProcess = OpenProcess(
    PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    FALSE,
    processId.value,
  );
  
  final exePath = calloc<WCHAR>(MAX_PATH);
  GetModuleFileNameEx(hProcess, 0, exePath, MAX_PATH);
  
  final path = exePath.toDartString();
  CloseHandle(hProcess);
  calloc.free(processId);
  calloc.free(exePath);
  
  return path.split('\\').last.replaceAll('.exe', '');
}
```

---

## 六、测试策略

### 6.1 单元测试
- 搜索算法测试（exact/fuzzy/regexp/mixed）
- 过滤逻辑测试（应用/类型/正则）
- 排序逻辑测试
- 去重逻辑测试

### 6.2 集成测试
- 剪贴板监听流程
- 数据持久化流程
- 快捷键响应
- 窗口显示/隐藏

### 6.3 手动测试清单
- [ ] 复制文本 → 显示在历史
- [ ] 复制图片 → 显示缩略图
- [ ] 复制文件 → 显示文件路径
- [ ] 搜索功能 → 高亮匹配
- [ ] 固定项 → 快捷键可用
- [ ] 粘贴 → 格式保留
- [ ] 去格式粘贴 → 纯文本
- [ ] 应用过滤 → 忽略生效
- [ ] 正则过滤 → 匹配忽略
- [ ] 多屏幕 → 正确定位

---

## 七、性能优化建议

### 7.1 数据库优化
```sql
-- 为常用查询添加索引
CREATE INDEX idx_last_copied ON history_items(last_copied_at DESC);
CREATE INDEX idx_pin ON history_items(pin) WHERE pin IS NOT NULL;
CREATE INDEX idx_application ON history_items(application);
```

### 7.2 UI 优化
- 使用 `ListView.builder` 懒加载
- 图片缩略图异步加载
- 搜索防抖（200ms）
- 虚拟滚动（仅渲染可见项）

### 7.3 内存优化
- 图片数据压缩存储
- 大文本截断（超过 10k 字符）
- 定期清理过期数据
- 使用 WeakReference 缓存

---

## 八、下一步行动

### 立即实施（本周）
1. 实现 `PasteService` - 多种粘贴模式
2. 实现 `ClipboardFilterService` - 应用和正则过滤
3. 完善 `SearchService` - 添加 mixed 模式
4. 实现项目快捷键监听（1-10）

### 短期目标（2周内）
1. 固定项系统完整实现
2. 排序策略切换
3. 预览弹窗
4. 高亮匹配显示

### 中期目标（1个月内）
1. 视觉效果优化（毛玻璃、动画）
2. 图片 OCR 集成
3. 完整的设置界面
4. 性能优化和测试

---

## 附录：Maccy 核心文件清单

### 必读文件（理解核心逻辑）
1. `Maccy/AppDelegate.swift` - 应用生命周期
2. `Maccy/Clipboard.swift` - 剪贴板监听核心
3. `Maccy/Models/HistoryItem.swift` - 数据模型
4. `Maccy/Observables/History.swift` - 历史管理
5. `Maccy/Search.swift` - 搜索引擎
6. `Maccy/Sorter.swift` - 排序逻辑
7. `Maccy/FloatingPanel.swift` - 弹窗管理

### 参考文件（实现细节）
1. `Maccy/Extensions/Defaults.Keys+Names.swift` - 所有配置项
2. `Maccy/HistoryItemAction.swift` - 操作类型
3. `Maccy/KeyShortcut.swift` - 快捷键生成
4. `Maccy/PopupPosition.swift` - 定位策略
5. `Maccy/Views/HistoryListView.swift` - 列表 UI
6. `Maccy/Views/HistoryItemView.swift` - 列表项 UI

---

**文档版本**: 1.0  
**最后更新**: 2026-05-09  
**基于**: Maccy 2.6.1 源码分析
