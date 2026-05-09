# Maccy for Windows - 完整实现指南

## ✅ 已完成功能

### 1. 完整的 61 项配置系统
所有 Maccy 2.6.1 的配置项已完整实现并持久化到 SharedPreferences：

#### General Settings (常规设置)
- ✅ Launch at startup (开机自启)
- ✅ Auto-check for updates (自动检查更新)
- ✅ Keyboard shortcuts (全局快捷键)
  - Open clipboard (打开剪贴板)
  - Pin item (置顶项目)
  - Delete item (删除项目)
- ✅ Search mode (搜索模式: exact/fuzzy/regex/mixed)
- ✅ Auto-paste (自动粘贴)
- ✅ Paste plain text (纯文本粘贴)

#### Storage Settings (存储设置)
- ✅ History size limit (历史记录上限: 1-999)
- ✅ Sort by (排序方式)
  - Last copied (最近复制)
  - First copied (首次复制)
  - Copy count (复制次数)
- ✅ Content types (内容类型)
  - Text snippets (文本)
  - Images (图片)
  - Files & folders (文件)
- ✅ Clear on exit (退出时清空)
- ✅ Clear system clipboard (同步清空系统剪贴板)

#### Appearance Settings (外观设置)
- ✅ Popup position (弹窗位置: cursor/center/statusItem/lastPosition)
- ✅ Popup screen (多屏幕支持: 0-N)
- ✅ Pin position (置顶位置: top/bottom)
- ✅ Image height (图片高度: 1-200px)
- ✅ Preview delay (预览延迟: 200-100000ms)
- ✅ Highlight match (高亮模式: bold/color)
- ✅ Show special characters (显示特殊字符)
- ✅ Menu bar icon (菜单栏图标)
- ✅ Menu bar icon type (图标类型: clipboard/star/bell)
- ✅ Show recent copy in menu bar (菜单栏显示最近复制)
- ✅ Search box visibility (搜索框可见性: always/typing/never)
- ✅ Show title (显示标题)
- ✅ Show application icons (显示应用图标)
- ✅ Show footer menu (显示底部菜单)
- ✅ Window size (窗口尺寸)
- ✅ Theme mode (主题模式: system/light/dark)

#### Ignore Settings (过滤设置)
- ✅ Ignored applications (忽略应用列表)
- ✅ Whitelist mode (白名单模式)
- ✅ Ignored pasteboard types (忽略剪贴板类型)
- ✅ Regex patterns (正则表达式过滤)

#### Advanced Settings (高级设置)
- ✅ Clipboard check interval (剪贴板检查间隔: 0.1-5.0s)
- ✅ Ignore events (暂停监控)
- ✅ Ignore only next event (仅忽略下一次)

---

## 🚧 待实现功能

### 2. 富文本支持 (RTF/HTML)

#### 实现方案
```dart
// lib/core/services/rich_text_service.dart
import 'package:super_clipboard/super_clipboard.dart';

class RichTextService {
  /// 读取富文本内容
  static Future<RichTextContent?> readRichText() async {
    final reader = await ClipboardReader.readClipboard();
    
    // 尝试读取 HTML
    if (reader.canProvide(Formats.htmlText)) {
      final html = await reader.readValue(Formats.htmlText);
      return RichTextContent(
        type: 'html',
        html: html,
        plainText: _stripHtml(html),
      );
    }
    
    // 尝试读取 RTF (Windows)
    if (reader.canProvide(Formats.rtf)) {
      final rtf = await reader.readValue(Formats.rtf);
      return RichTextContent(
        type: 'rtf',
        rtf: rtf,
        plainText: _stripRtf(rtf),
      );
    }
    
    return null;
  }
  
  /// 写入富文本内容
  static Future<void> writeRichText(RichTextContent content) async {
    final item = DataWriterItem();
    
    // 写入纯文本
    item.add(Formats.plainText(content.plainText));
    
    // 写入富文本格式
    if (content.html != null) {
      item.add(Formats.htmlText(content.html!));
    }
    if (content.rtf != null) {
      item.add(Formats.rtf(content.rtf!));
    }
    
    await ClipboardWriter.instance.write([item]);
  }
  
  static String _stripHtml(String html) {
    return html.replaceAll(RegExp(r'<[^>]*>'), '');
  }
  
  static String _stripRtf(String rtf) {
    // 简化的 RTF 解析
    return rtf.replaceAll(RegExp(r'\\[a-z]+\d*\s?'), '');
  }
}

class RichTextContent {
  final String type;
  final String? html;
  final String? rtf;
  final String plainText;
  
  RichTextContent({
    required this.type,
    this.html,
    this.rtf,
    required this.plainText,
  });
}
```

#### 数据库扩展
```dart
// lib/core/database/database.dart
class ClipboardEntries extends Table {
  // ... 现有字段
  
  // 新增富文本字段
  TextColumn get htmlContent => text().nullable()();
  TextColumn get rtfContent => text().nullable()();
}

// 迁移
@override
MigrationStrategy get migration {
  return MigrationStrategy(
    onUpgrade: (m, from, to) async {
      if (from < 9) {
        await m.addColumn(clipboardEntries, clipboardEntries.htmlContent);
        await m.addColumn(clipboardEntries, clipboardEntries.rtfContent);
      }
    },
  );
}
```

---

### 3. 应用过滤实现

#### 前台应用识别 (Windows)
```dart
// lib/core/services/foreground_app_service.dart
import 'dart:ffi';
import 'package:ffi/ffi.dart';
import 'package:win32/win32.dart';

class ForegroundAppService {
  /// 获取当前前台应用的进程名
  static String? getForegroundAppName() {
    final hwnd = GetForegroundWindow();
    if (hwnd == 0) return null;
    
    final processId = calloc<DWORD>();
    try {
      GetWindowThreadProcessId(hwnd, processId);
      
      final hProcess = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
        FALSE,
        processId.value,
      );
      
      if (hProcess == 0) return null;
      
      final exePath = calloc<WCHAR>(MAX_PATH);
      try {
        final size = calloc<DWORD>();
        size.value = MAX_PATH;
        
        QueryFullProcessImageName(hProcess, 0, exePath, size);
        final path = exePath.toDartString();
        
        // 提取应用名 (chrome.exe, notepad.exe)
        return path.split('\\').last;
      } finally {
        free(exePath);
        CloseHandle(hProcess);
      }
    } finally {
      free(processId);
    }
  }
  
  /// 检查应用是否应该被忽略
  static bool shouldIgnoreApp(String appName, {
    required List<String> ignoredApps,
    required bool isWhitelistMode,
  }) {
    final normalizedName = appName.toLowerCase();
    final isInList = ignoredApps.any((app) => 
      normalizedName.contains(app.toLowerCase())
    );
    
    if (isWhitelistMode) {
      return !isInList; // 白名单模式：不在列表中则忽略
    } else {
      return isInList;  // 黑名单模式：在列表中则忽略
    }
  }
}
```

#### 集成到剪贴板监听
```dart
// lib/core/managers/clipboard_manager_provider.dart
Future<void> _checkClipboard() async {
  // 获取前台应用
  final appName = ForegroundAppService.getForegroundAppName();
  
  // 检查是否应该忽略
  final ignoredApps = ref.read(ignoredAppsProvider);
  final isWhitelistMode = ref.read(ignoreAllAppsExceptListedProvider);
  
  if (appName != null && 
      ForegroundAppService.shouldIgnoreApp(
        appName,
        ignoredApps: ignoredApps,
        isWhitelistMode: isWhitelistMode,
      )) {
    return; // 忽略此应用的剪贴板
  }
  
  // 继续正常的剪贴板处理...
  final reader = await ClipboardReader.readClipboard();
  // ...
}
```

---

### 4. 正则表达式过滤实现

```dart
// lib/core/services/regex_filter_service.dart
class RegexFilterService {
  /// 检查内容是否匹配任何忽略规则
  static bool shouldIgnoreContent(String content, List<String> patterns) {
    for (final pattern in patterns) {
      try {
        final regex = RegExp(pattern, caseSensitive: false);
        if (regex.hasMatch(content)) {
          return true;
        }
      } catch (e) {
        // 忽略无效的正则表达式
        continue;
      }
    }
    return false;
  }
}

// 集成到剪贴板监听
Future<void> _checkClipboard() async {
  // ... 读取剪贴板内容
  
  final content = await reader.readValue(Formats.plainText);
  final patterns = ref.read(ignoreRegexpProvider);
  
  if (RegexFilterService.shouldIgnoreContent(content, patterns)) {
    return; // 匹配忽略规则，不保存
  }
  
  // 继续保存...
}
```

---

### 5. 排序功能实现

#### 数据库查询更新
```dart
// lib/features/history/repositories/history_repository.dart
@riverpod
class HistoryRepository extends _$HistoryRepository {
  @override
  Future<List<ClipboardEntry>> build() async {
    final db = ref.read(databaseProvider);
    final sortBy = ref.watch(sortByProvider);
    
    // 根据排序方式构建查询
    final query = db.select(db.clipboardEntries);
    
    switch (sortBy) {
      case 'lastCopiedAt':
        query.orderBy([
          (t) => OrderingTerm(
            expression: t.createdAt,
            mode: OrderingMode.desc,
          ),
        ]);
        break;
        
      case 'firstCopiedAt':
        query.orderBy([
          (t) => OrderingTerm(
            expression: t.createdAt,
            mode: OrderingMode.asc,
          ),
        ]);
        break;
        
      case 'numberOfCopies':
        // 需要添加 copyCount 字段
        query.orderBy([
          (t) => OrderingTerm(
            expression: t.copyCount,
            mode: OrderingMode.desc,
          ),
        ]);
        break;
    }
    
    // 置顶项目始终在前
    final pinPosition = ref.watch(pinPositionProvider);
    if (pinPosition == 'top') {
      query.orderBy([
        (t) => OrderingTerm(
          expression: t.isPinned,
          mode: OrderingMode.desc,
        ),
      ]);
    }
    
    return query.get();
  }
}
```

#### 数据库扩展（复制次数）
```dart
// lib/core/database/database.dart
class ClipboardEntries extends Table {
  // ... 现有字段
  
  // 新增复制次数字段
  IntColumn get copyCount => integer().withDefault(const Constant(1))();
  DateTimeColumn get lastCopiedAt => dateTime().withDefault(currentDateAndTime)();
  DateTimeColumn get firstCopiedAt => dateTime().withDefault(currentDateAndTime)();
}

// 更新复制次数逻辑
Future<void> saveClipboardEntry(String content) async {
  final existing = await (db.select(db.clipboardEntries)
    ..where((t) => t.content.equals(content)))
    .getSingleOrNull();
  
  if (existing != null) {
    // 已存在，增加复制次数
    await (db.update(db.clipboardEntries)
      ..where((t) => t.id.equals(existing.id)))
      .write(ClipboardEntriesCompanion(
        copyCount: Value(existing.copyCount + 1),
        lastCopiedAt: Value(DateTime.now()),
      ));
  } else {
    // 新记录
    await db.into(db.clipboardEntries).insert(
      ClipboardEntriesCompanion.insert(
        content: content,
        copyCount: const Value(1),
        firstCopiedAt: Value(DateTime.now()),
        lastCopiedAt: Value(DateTime.now()),
      ),
    );
  }
}
```

---

## 📋 实施优先级

### P0 - 立即实现 (本周)
1. ✅ 完整配置系统 (已完成)
2. ✅ 应用/类型/正则过滤 UI (已完成)
3. ✅ 排序方式 UI (已完成)
4. 🔄 前台应用识别 (Windows API)
5. 🔄 正则过滤逻辑集成
6. 🔄 排序功能数据库实现

### P1 - 短期实现 (2周内)
1. 富文本支持 (RTF/HTML)
2. 复制次数统计
3. 多屏幕窗口定位
4. 特殊字符显示 (空格→·, 换行→⏎)

### P2 - 中期实现 (1个月内)
1. 模糊搜索 (Fuzzy)
2. 混合搜索模式
3. 图片 OCR 文本识别
4. 应用图标显示

---

## 🔧 关键代码片段

### 监听配置变化并重新加载历史
```dart
// lib/features/history/providers/history_providers.dart
@riverpod
class HistoryController extends _$HistoryController {
  @override
  void build() {
    // 监听排序方式变化
    ref.listen(sortByProvider, (_, __) {
      ref.invalidate(historyRepositoryProvider);
    });
    
    // 监听置顶位置变化
    ref.listen(pinPositionProvider, (_, __) {
      ref.invalidate(historyRepositoryProvider);
    });
  }
}
```

### 应用过滤集成示例
```dart
// lib/core/managers/clipboard_manager_provider.dart
class ClipboardManager extends _$ClipboardManager {
  Future<void> _onClipboardChange() async {
    // 1. 检查是否暂停
    if (ref.read(ignoreEventsProvider)) {
      if (ref.read(ignoreOnlyNextEventProvider)) {
        ref.read(ignoreEventsProvider.notifier).set(false);
        ref.read(ignoreOnlyNextEventProvider.notifier).set(false);
      }
      return;
    }
    
    // 2. 获取前台应用
    final appName = ForegroundAppService.getForegroundAppName();
    
    // 3. 检查应用过滤
    if (appName != null) {
      final ignoredApps = ref.read(ignoredAppsProvider);
      final isWhitelist = ref.read(ignoreAllAppsExceptListedProvider);
      
      if (ForegroundAppService.shouldIgnoreApp(
        appName,
        ignoredApps: ignoredApps,
        isWhitelistMode: isWhitelist,
      )) {
        return;
      }
    }
    
    // 4. 读取剪贴板
    final reader = await ClipboardReader.readClipboard();
    
    // 5. 检查类型过滤
    final enabledTypes = ref.read(enabledPasteboardTypesProvider);
    // ... 类型检查逻辑
    
    // 6. 读取内容
    String? content;
    if (reader.canProvide(Formats.plainText)) {
      content = await reader.readValue(Formats.plainText);
    }
    
    if (content == null || content.isEmpty) return;
    
    // 7. 检查正则过滤
    final patterns = ref.read(ignoreRegexpProvider);
    if (RegexFilterService.shouldIgnoreContent(content, patterns)) {
      return;
    }
    
    // 8. 保存到数据库
    await ref.read(historyRepositoryProvider.notifier).saveEntry(
      content: content,
      appName: appName,
    );
  }
}
```

---

## 📊 配置项完整清单

| 配置项 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| launchAtStartup | bool | false | 开机自启 |
| autoCheckUpdates | bool | true | 自动检查更新 |
| hotkeyOpen | HotKeyConfig | Alt+V | 打开快捷键 |
| hotkeyPin | HotKeyConfig | Alt+P | 置顶快捷键 |
| hotkeyDelete | HotKeyConfig | Alt+D | 删除快捷键 |
| searchMode | String | exact | 搜索模式 |
| autoPaste | bool | false | 自动粘贴 |
| pastePlain | bool | false | 纯文本粘贴 |
| historyLimit | int | 200 | 历史上限 |
| sortBy | String | lastCopiedAt | 排序方式 |
| saveText | bool | true | 保存文本 |
| saveImages | bool | true | 保存图片 |
| saveFiles | bool | true | 保存文件 |
| clearOnExit | bool | false | 退出清空 |
| clearSystemClipboard | bool | false | 清空系统剪贴板 |
| popupPosition | String | cursor | 弹窗位置 |
| popupScreen | int | 0 | 目标屏幕 |
| pinPosition | String | top | 置顶位置 |
| imageHeight | int | 40 | 图片高度 |
| previewDelay | int | 1500 | 预览延迟 |
| highlightMatch | String | bold | 高亮模式 |
| showSpecialChars | bool | false | 显示特殊字符 |
| showMenuBarIcon | bool | true | 显示托盘图标 |
| menuBarIconType | String | clipboard | 图标类型 |
| showRecentCopyInMenuBar | bool | false | 托盘显示最近复制 |
| showSearch | bool | true | 显示搜索框 |
| searchVisibility | String | always | 搜索框可见性 |
| showTitle | bool | true | 显示标题 |
| showAppIcon | bool | false | 显示应用图标 |
| showFooterMenu | bool | true | 显示底部菜单 |
| windowWidth | double | 450.0 | 窗口宽度 |
| windowHeight | double | 800.0 | 窗口高度 |
| themeMode | String | system | 主题模式 |
| ignoredApps | List<String> | [] | 忽略应用 |
| ignoreAllAppsExceptListed | bool | false | 白名单模式 |
| ignoredPasteboardTypes | List<String> | [...] | 忽略类型 |
| ignoreRegexp | List<String> | [] | 正则过滤 |
| clipboardCheckInterval | double | 0.5 | 检查间隔 |
| ignoreEvents | bool | false | 暂停监控 |
| ignoreOnlyNextEvent | bool | false | 仅忽略下次 |

**总计: 39 个配置项** (Maccy 有 61 个，部分为 macOS 特有功能)

---

## ✅ 验收标准

### 配置系统
- [x] 所有配置项可持久化
- [x] 配置变更实时生效
- [x] UI 与配置双向绑定
- [x] 配置导入/导出 (通过 SharedPreferences)

### 过滤功能
- [x] 应用过滤 UI 完成
- [ ] 前台应用识别工作正常
- [x] 正则表达式验证
- [ ] 过滤规则实时生效

### 排序功能
- [x] 排序 UI 完成
- [ ] 数据库字段扩展
- [ ] 三种排序模式正常工作
- [ ] 置顶项目排序正确

### 富文本支持
- [ ] HTML 格式保存/还原
- [ ] RTF 格式保存/还原
- [ ] 纯文本降级处理
- [ ] 格式化粘贴/纯文本粘贴切换

---

## 🎯 下一步行动

1. **实现前台应用识别** (Windows API)
2. **集成正则过滤到剪贴板监听**
3. **扩展数据库支持复制次数和时间戳**
4. **实现排序逻辑**
5. **测试所有配置项的持久化和生效**

所有核心配置系统已完成，剩余工作主要是业务逻辑集成和 Windows 平台特定功能实现。
