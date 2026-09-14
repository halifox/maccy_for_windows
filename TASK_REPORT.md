# Maccy 2.7.1 设置页 Windows 移植任务报告

## 1. 任务范围

本次实现以 `Maccy-2.7.1/Maccy/Settings` 下的六个设置页为功能基准，并结合以下源码确认设置实际影响的行为：

- `GeneralSettingsPane.swift`
- `AppearanceSettingsPane.swift`
- `StorageSettingsPane.swift`
- `IgnoreSettingsPane/*`
- `PinsSettingsPane.swift`
- `AdvancedSettingsPane.swift`
- `Clipboard.swift`
- `Extensions/Defaults.Keys+Names.swift`

Windows 端没有引入资源编辑器、复杂 UI 框架或重量级组件；设置窗口使用 WTL 的 `CWindowImpl`、Tab、Edit、ComboBox、ListBox、CheckBox 和 HotKey 控件直接创建。

## 2. 已实现功能

### 通用

- 登录 Windows 自动启动：写入当前用户 `Run` 注册表项。
- Open、Pin、Delete、Preview 四组热键设置和持久化。
- 精确、模糊、正则、混合搜索。
- 选择后自动粘贴。
- 默认去除格式：保留纯文本和文件，过滤 HTML/RTF 等富文本格式。
- 自动检查更新开关：保存到 Windows 设置数据库；Windows 版暂未实现后台下载、签名校验和自动安装。
- 检查更新按钮：打开 Maccy GitHub 的 latest release 页面。
- Windows 通知设置入口：打开 `ms-settings:notifications`。

### 外观

- 光标、托盘图标、目标窗口中心、屏幕中心、上次位置。
- 多显示器选择和上次位置重置。
- 置顶项目显示在顶部或底部。
- 图片最大显示高度和预览延迟。
- 匹配高亮：颜色、粗体、斜体、下划线。
- 托盘图标显示/隐藏、图标样式选择、最近复制内容提示。
- 搜索框显示方式：始终显示或搜索时显示。
- 来源程序图标、十六进制颜色色块和特殊符号。

### 存储

- 文本：Unicode、ANSI、HTML、RTF。
- 文件：`CF_HDROP`。
- 图片：DIB/DIBV5。
- 历史数量限制 1–999，置顶项目不占用该限制。
- 最近复制、首次复制、复制次数排序。
- 显示 SQLite 数据库及 WAL 文件占用。

### 忽略

- 按 Windows `.exe` 完整路径或文件名忽略应用。
- 应用白名单模式：只记录列表中的应用。
- 按 Windows 剪贴板格式名忽略内容。
- Unicode 文本正则表达式忽略。
- 忽略应用、格式和正则列表的添加、修改、删除。
- 格式列表恢复 Maccy 默认规则：
  `Pasteboard generator type`、`com.agilebits.onepassword`、
  `com.typeit4me.clipping`、`de.petermaurer.TransientPasteboardType`、
  `net.antelle.keeweb`。

### 置顶

- 查看置顶记录、修改按键和标题、删除记录。
- 纯文本内容可编辑；保存时只保留 Unicode 文本格式，与 Maccy 的去富文本行为一致。
- 图片、文件记录允许修改元数据，但不在设置页编辑二进制内容。

### 高级

- 暂时忽略新的剪贴板事件。
- 只忽略下一次事件，完成后自动关闭。
- 退出时清空未置顶历史。
- 清空历史时同步清空系统剪贴板。

## 3. 数据与低内存设计

- 数据库：`%LOCALAPPDATA%\Clipboard\app.db`。
- `history_items` 只保存搜索和列表所需的元数据；实际剪贴板格式存放在 `history_data` BLOB 表中。
- 列表元数据中的文本预览最多 4096 个字符；长文本的完整内容仍在 BLOB 中，避免打开历史面板时按条目复制大文本。
- 历史列表只加载元数据；用户恢复、预览或编辑某条记录时才读取对应 BLOB。
- SQLite 使用 WAL、`synchronous=NORMAL`、内存临时表和 1 秒 busy timeout。
- 单次剪贴板捕获限制为 32 MiB，文本限制为 1 MiB，防止异常数据造成常驻内存增长。
- 旧版项目的 `clipboard_history` 表会自动迁移到新表结构，旧文本内容不会丢失。

## 4. 构建验证

已使用 Visual Studio 18 Community 的 x64 MSVC 工具链验证：

```text
cmake --build cmake-build-debug --config Debug --parallel 2
```

结果：Debug 构建成功，`clipboard.exe` 链接成功。

Release 构建使用相同的 CMake 配置和 MSVC 工具链，可执行以下命令复验：

```text
cmake --build cmake-build-release --config Release --parallel 2
```

## 5. Windows 平台不支持或只能等价实现的功能

以下差异来自 macOS API 与 Windows API 的平台边界，并非 WTL 控件缺失：

1. **Sparkle 自动更新**：Windows 版没有实现后台下载、签名校验和自动安装。自动检查更新开关会保存，检查更新按钮打开 GitHub 发布页。
2. **macOS 通知声音**：通知入口映射到 Windows 通知设置；Maccy 的 per-copy `knock` 声音没有一比一实现。
3. **菜单栏语义**：Windows 没有 macOS menu bar。`showRecentCopyInMenuBar` 等价实现为通知区域图标 Tooltip；菜单图标使用 Windows 系统图标映射，不使用 macOS SF Symbols/NSImage 资源。
4. **NSPasteboard 类型**：Windows 没有 `NSPasteboard.PasteboardType` 的统一类型集合。当前实现支持 Unicode/ANSI/HTML/RTF、`CF_HDROP`、DIB/DIBV5 及注册格式名；动态 macOS 类型、Apple 专用类型和 `CF_BITMAP` 原生句柄没有跨平台等价的持久化表示。
5. **图片预览与 Vision OCR**：图片数据可以保存、恢复和参与历史记录，但没有 macOS Vision OCR；列表中显示图片占位信息，未实现 Maccy 的 OCR 标题提取和原图缩略图渲染。
6. **应用标识**：macOS 使用 Bundle Identifier；Windows 忽略应用使用进程 `.exe` 路径/文件名，因此不能直接共享两端的应用列表值。
7. **Win 修饰键录入**：Windows 原生 HotKey 控件可稳定录入 Ctrl、Alt、Shift；设置页没有暴露 Win 修饰键录入。底层 `RegisterHotKey` 能接受 Win 修饰键，但需要后续增加自定义录入控件才能完整覆盖。
8. **跨权限粘贴**：普通权限目标窗口通常可以通过 `SendInput(Ctrl+V)` 粘贴；管理员权限窗口、UIPI 隔离窗口、安全桌面和部分远程桌面场景会被 Windows 拒绝，这是系统安全边界。
9. **macOS 专用控制方式**：Option 点击状态项、Shell Script 开关和 macOS Accessibility/CGEvent 行为没有逐项复制；Windows 版提供设置窗口、托盘菜单、全局热键和 Windows 输入注入的等价路径。

除上述平台差异外，六个设置页的状态保存、列表编辑和对主程序行为的联动已经接通。
