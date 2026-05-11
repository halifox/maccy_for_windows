# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概述

Maccy 是一款使用 Flutter 构建的跨平台剪贴板管理器，灵感来自 macOS 上的 Maccy。当前主要针对 Windows 平台开发，使用 Riverpod 进行状态管理，Drift (SQLite) 作为本地数据库，GoRouter 处理路由。

**核心特性**:
- 多格式剪贴板支持 (文本、HTML、RTF、图片、文件)
- 四种搜索模式 (精确、模糊、正则、混合)
- Pin 快捷键 (b-y，排除 a/q/v/w/z)
- 全局热键唤醒 + 循环选择模式 (Cycle Mode)
- 窗口位置智能计算 (光标跟随、托盘跟随、屏幕中央、记忆位置)
- 自动去重逻辑 (基于内容哈希)

## 架构

### 目录结构

```
lib/
├── main.dart                    # 应用入口，初始化 WindowManager 和 SharedPreferences
├── app.dart                     # 根 Widget，配置 GoRouter 和主题，包含 BlankPage/SplashScreen/ErrorScreen
├── core/                        # 核心基础设施层
│   ├── database/                # Drift 数据库定义和 provider
│   │   ├── database.dart        # 表定义 + 自定义 regexp 函数
│   │   └── database_provider.dart
│   ├── managers/                # 全局管理器 (使用 @Riverpod(keepAlive: true))
│   │   ├── initialization_provider.dart  # 启动编排器，并行初始化所有 manager
│   │   ├── clipboard_manager_provider.dart  # 剪贴板监听 + 多格式存储
│   │   ├── hotkey_manager_provider.dart     # 全局热键 + 三态状态机 (toggle/opening/cycle)
│   │   ├── window_manager_provider.dart     # 窗口显示/隐藏 + 位置计算
│   │   ├── tray_manager_provider.dart       # 系统托盘
│   │   └── launch_manager_provider.dart     # 开机自启动
│   ├── services/                # 业务服务 (纯函数或轻量类)
│   │   ├── search_service.dart           # 四种搜索模式实现
│   │   ├── advanced_search_service.dart  # 混合搜索
│   │   ├── paste_service.dart            # 粘贴模拟
│   │   ├── pin_service.dart              # Pin 快捷键管理
│   │   ├── clipboard_filter_service.dart # 应用黑名单/白名单过滤
│   │   ├── foreground_app_service.dart   # 获取前台应用名称 (Windows)
│   │   ├── screen_service.dart           # 窗口位置计算
│   │   ├── rich_text_service.dart        # RTF 格式读取 (Windows)
│   │   └── modifier_key_service.dart     # 修饰键监听 (用于 Cycle Mode)
│   ├── models/                  # 数据模型
│   ├── utils/                   # 工具函数
│   └── constants/               # UI 常量
└── features/                    # 功能模块
    ├── history/                 # 剪贴板历史记录功能
    │   ├── ui/                  # 历史页面和组件
    │   │   ├── history_page.dart        # 主界面 (搜索框 + 列表 + 底部菜单)
    │   │   └── widgets/                 # 预览弹窗、快捷键显示等
    │   ├── providers/           # Riverpod providers
    │   │   └── history_providers.dart   # HistoryController + 搜索/选中状态
    │   └── repositories/        # 数据仓库层
    │       └── history_repository.dart  # 数据库操作 + 去重逻辑
    └── settings/                # 设置功能
        ├── ui/                  # 设置页面和标签页
        └── providers/           # 设置相关 providers (SharedPreferences 封装)

windows/runner/                  # Windows 原生代码
├── flutter_window.cpp           # MethodChannel 实现 (recordActiveApp, restoreAndPaste)
└── utils.cpp                    # Windows API 封装
```

### 核心概念

#### 1. 数据库架构 (`lib/core/database/database.dart`)

**表结构**:
- `HistoryItems`: 剪贴板条目元数据
  - `id`: 主键
  - `application`: 来源应用 (bundle ID 或进程名)
  - `firstCopiedAt` / `lastCopiedAt`: 时间戳
  - `numberOfCopies`: 复制次数 (用于去重时累加)
  - `pin`: 固定快捷键 (单字符 b-y，null 表示未固定)
  - `title`: 显示标题 (最长 1000 字符)

- `HistoryItemContents`: 条目内容 (一对多)
  - `itemId`: 外键，级联删除
  - `type`: 内容类型 (`text/plain`, `text/html`, `text/rtf`, `image/png`, `image/jpeg`, `file`)
  - `value`: 二进制数据 (BLOB)

**自定义函数**:
- `regexp(pattern, input)`: SQLite 正则表达式函数，用于 `SearchMode.regexp`

**去重逻辑** (实现 Maccy 的 `supersedes` 方法):
1. 新内容到达时，查找所有现有条目
2. 比较非临时类型 (排除 `com.apple.pasteboard.*` 等)
3. 如果所有非临时内容都匹配，则更新 `lastCopiedAt` 和 `numberOfCopies`，而非插入新条目

#### 2. 启动流程 (`lib/core/managers/initialization_provider.dart`)

```dart
@riverpod
Future<void> appStartup(Ref ref) async {
  await Future.wait([
    ref.watch(appWindowManagerProvider.future),
    ref.watch(appTrayManagerProvider.future),
    ref.watch(appHotKeyManagerProvider.future),
    ref.watch(appClipboardManagerProvider.future),
    ref.watch(appLaunchManagerProvider.future),
  ]);
}
```

- 所有 manager 并行初始化
- 完成前显示 `SplashScreen`
- 失败时显示 `ErrorScreen` (可重试)

#### 3. 剪贴板管理器 (`lib/core/managers/clipboard_manager_provider.dart`)

**核心流程**:
1. 监听 `clipboard_watcher` 的 `onClipboardChanged` 事件
2. 使用 200ms 防抖 (避免连续触发)
3. 检查是否暂停监听 (`ignoreEventsProvider`)
4. 获取前台应用名称，执行黑名单/白名单过滤
5. 读取剪贴板所有格式 (text/plain, text/html, text/rtf, image/png, image/jpeg, file)
6. 对文本内容执行正则过滤 (`ignoreRegexpProvider`)
7. 生成标题 (优先级: 文件名 > 文本 > `[图片]`)
8. 调用 `repository.addOrUpdateEntry` 保存 (自动去重)

**自身更新标志**:
- `isSelfUpdate = true`: 当应用自己修改剪贴板时设置，避免重复处理
- 用于 `selectItem` 后的粘贴操作

#### 4. 热键管理器 (`lib/core/managers/hotkey_manager_provider.dart`)

**三态状态机** (实现 Maccy 的 Cycle Mode):

```
toggle: 按一次打开，再按关闭
   ↓ (按下热键，窗口未显示)
opening: 刚打开窗口，等待 200ms
   ↓ (200ms 内再次按键)        ↓ (200ms 后无操作)
cycle: 循环选择模式              toggle
   ↓ (释放修饰键)
   自动粘贴选中项并关闭
```

**实现细节**:
- `_handleHotkeyPressed()`: 处理热键按下
- `_startModifierMonitoring()`: 开始监听修饰键释放 (使用 `ModifierKeyService`)
- `_handleModifiersReleased()`: 修饰键释放时自动粘贴

#### 5. 窗口管理器 (`lib/core/managers/window_manager_provider.dart`)

**窗口位置计算** (委托给 `ScreenService`):
- `cursor`: 光标位置 (使用 Windows API `GetCursorPos`)
- `center`: 屏幕中央
- `statusItem`: 托盘图标位置 (使用 `tray_manager` 获取托盘坐标)
- `lastPosition`: 记忆上次位置 (保存在 `SharedPreferences`)

**失焦自动隐藏**:
- 实现 `WindowListener.onWindowBlur()`
- 窗口失去焦点时自动调用 `hideHistory()`

**防抖逻辑**:
- `_lastHideTime`: 记录上次隐藏时间
- 200ms 内不响应显示请求 (避免托盘点击与失焦事件冲突)

#### 6. 搜索服务 (`lib/core/services/search_service.dart`)

**四种模式**:
- `exact`: 不区分大小写的子串匹配 (`String.contains`)
- `fuzzy`: 模糊搜索 (使用 `fuzzy` 包，threshold: 0.3，限制 5000 字符)
- `regexp`: 正则表达式 (使用 Dart `RegExp`)
- `mixed`: 混合模式 (依次尝试 exact → regexp → fuzzy)

**性能优化**:
- 超长文本 (> 5000 字符) 截断后再进行模糊搜索
- 搜索在内存中执行 (数据库返回全量，然后过滤)

#### 7. 历史记录控制器 (`lib/features/history/providers/history_providers.dart`)

**核心方法**:
- `selectItem(index)`: 将条目写入剪贴板并模拟粘贴
  - 读取所有内容格式 (`repository.getItemContents`)
  - 使用 `super_clipboard` 写入多格式数据
  - 设置 `isSelfUpdate = true` 避免重复监听
  - 调用 `simulatePaste()` (通过 MethodChannel 调用 Windows API)

- `selectNext/selectPrevious`: 键盘导航 (支持循环)
- `togglePin`: 切换固定状态
- `deleteItem`: 删除条目
- `clearHistory`: 清空所有历史 (可选清空系统剪贴板)

**键盘事件处理**:
- `handleKeyEvent`: 统一处理所有键盘事件
  - 上下键: 导航
  - Enter: 选择并粘贴
  - Delete: 删除
  - Alt+数字: 快速选择 (0-9 对应前 10 项)
  - Alt+字母: Pin 快捷键 (b-y)

## 开发命令

### 代码生成

```bash
# 运行 build_runner 生成 Riverpod 和 Drift 代码
dart run build_runner build --delete-conflicting-outputs

# 监听模式 (开发时推荐)
dart run build_runner watch --delete-conflicting-outputs
```

**重要**: `build.yaml` 配置了两个独立的 target:
- `drift_target`: 仅为 `database.dart` 生成 Drift 代码
- `$default`: 为其他文件生成 Riverpod 代码

修改 `@riverpod` 注解或 Drift 表定义后必须重新运行 build_runner。

### 运行和构建

```bash
# 运行应用 (Windows)
flutter run -d windows

# 构建 Windows 发布版本
flutter build windows --release

# 运行测试
flutter test

# 代码分析
flutter analyze
```

### Linting

项目使用严格的 lint 规则 (`analysis_options.yaml`):
- 强制使用单引号
- 优先使用 `const` 构造函数
- 强制使用 `package:` 导入 (不使用相对导入)
- 构造函数必须在类的最前面
- 严格类型推断 (`strict-casts`, `strict-inference`, `strict-raw-types`)

生成的 `*.g.dart` 和 `*.freezed.dart` 文件已排除在分析之外。

## 开发注意事项

### 数据库迁移

当前 schema 版本为 12。修改表结构时:
1. 更新 `database.dart` 中的表定义
2. 增加 `schemaVersion` 版本号
3. 在 `onUpgrade` 中添加迁移逻辑
4. 运行 `dart run build_runner build` 重新生成

### Pin 快捷键

Pin 功能使用单字符快捷键 (b-y)，排除了 a/q/v/w/z (系统保留):
- `a`: 全选 (Ctrl+A)
- `q`: 退出 (Alt+F4)
- `v`: 粘贴 (Ctrl+V)
- `w`: 关闭窗口 (Ctrl+W)
- `z`: 撤销 (Ctrl+Z)

相关逻辑在 `PinService` 和 `HistoryRepository.getNextAvailablePin()` 中。

### 搜索模式

`SearchService` 支持四种模式:
- `exact`: 精确匹配 (不区分大小写)
- `fuzzy`: 模糊搜索 (threshold: 0.3，限制 5000 字符)
- `regexp`: 正则表达式 (自定义 SQLite 函数)
- `mixed`: 混合模式 (依次尝试 exact → regexp → fuzzy)

搜索在内存中执行，数据库返回全量数据后再过滤。

### Windows 平台特定

**原生集成** (`windows/runner/flutter_window.cpp`):
- `recordActiveApp`: 记录当前活跃窗口 (用于粘贴后恢复焦点)
- `restoreAndPaste`: 恢复活跃窗口并模拟 Ctrl+V

**使用的包**:
- `win32` + `ffi`: Windows API 调用
- `window_manager`: 窗口管理 (无边框、置顶、阴影)
- `hotkey_manager`: 全局热键注册
- `clipboard_watcher`: 剪贴板监听
- `super_clipboard`: 多格式剪贴板读写
- `tray_manager`: 系统托盘
- `screen_retriever`: 屏幕信息获取

### Riverpod 使用模式

**Provider 类型**:
- `@Riverpod(keepAlive: true)`: 全局单例 (managers)
- `@riverpod`: 自动销毁 (controllers, repositories)
- `@riverpod Stream<T>`: 数据流 (filteredHistoryProvider)

**状态管理**:
- 简单状态: `StateProvider` 或 `@riverpod class XxxNotifier`
- 复杂逻辑: `@riverpod class XxxController`
- 数据流: `Stream<T>` provider (监听数据库变化)

**依赖注入**:
- 使用 `ref.watch()` 监听依赖
- 使用 `ref.read()` 一次性读取
- 使用 `ref.listen()` 监听变化并执行副作用

### 常见开发场景

**添加新的剪贴板格式**:
1. 在 `ClipboardManager._processClipboardChange()` 中添加格式检测
2. 在 `HistoryController.selectItem()` 中添加写入逻辑
3. 在 `HistoryRepository` 中更新去重逻辑 (如需要)

**添加新的搜索模式**:
1. 在 `SearchMode` 枚举中添加新模式
2. 在 `SearchService` 中实现搜索逻辑
3. 在 `HistoryRepository._parseSearchMode()` 中添加解析

**添加新的窗口位置模式**:
1. 在 `PopupPosition` 枚举中添加新模式
2. 在 `ScreenService.calculateWindowPosition()` 中实现计算逻辑
3. 在 `WindowManager._parsePopupPosition()` 中添加解析

**调试技巧**:
- 使用 `debugPrint()` 输出日志 (自动在 Release 模式下禁用)
- 使用 Flutter DevTools 查看 Riverpod 状态树
- 使用 Drift Inspector 查看数据库内容 (开发模式)
- 使用 `flutter run --verbose` 查看详细日志

### 代码风格

遵循用户的核心原则:
- 实现优先，过程式风格
- KISS 和 YAGNI 原则
- 少文件、少类、少包装、少中间层
- 避免过度抽象和不必要的注释
- 默认不写注释，仅在非显而易见的逻辑处添加

### 性能优化

**已实施的优化**:
- 剪贴板监听使用 200ms 防抖
- 搜索时超长文本截断 (5000 字符)
- 使用 `RepaintBoundary` 隔离列表重绘
- 使用 `select` 精细化控制 Rebuild
- 数据库查询使用 `limit` 限制返回数量
- 定期清理超出限制的历史记录

**注意事项**:
- 避免在 `build()` 方法中执行耗时操作
- 大列表使用 `ListView.builder` 而非 `ListView`
- 图片预览使用懒加载
- 数据库操作使用 `isolate` (Drift 自动处理)
