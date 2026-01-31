# 技术设计文档 - HaliClip Architecture

## 1. 技术栈选型
- **框架**: Flutter (Desktop)
- **状态管理**: `flutter_riverpod`, `hooks_riverpod`, `riverpod_annotation` (使用 Code Generation)
- **UI 逻辑**: `flutter_hooks` (Functional Widgets)
- **本地数据库**: `drift`, `drift_flutter` (强大的类型安全 SQL 数据库)
- **路由管理**: `go_router`
- **窗口管理**: `window_manager`
- **系统托盘**: `tray_manager`
- **热键支持**: `hotkey_manager`
- **剪贴板监听**: `clipboard_watcher`

## 2. 核心模块设计

### 2.1 状态管理与数据流
- 使用 `riverpod` 的 `@riverpod` 注解生成 Providers。
- `ClipboardProvider`: 负责通过 `clipboard_watcher` 接收信号，并使用 `Drift` 写入数据库，同时更新 UI 状态。
- `SettingsProvider`: 管理全局快捷键、开机自启等配置。

### 2.2 数据库设计 (Drift)
- 定义 `ClipboardEntries` 表：`id`, `content`, `type`, `createdAt`。
- 使用 `drift_dev` 生成类型安全的查询代码。

### 2.3 窗口交互流 (Maccy Logic)
1. **呼出**：获取 `Screen` 和 `Cursor` 坐标，计算窗口最佳弹出位置（防止溢出屏幕）。
2. **过滤**：使用 `Drift` 的 SQL 模糊查询或集成 `fuzzy` 插件在内存中进行高性能过滤。
3. **粘贴逻辑**：
   - 写入剪贴板：`Clipboard.setData`。
   - 自动粘贴 (Advanced)：在 Windows 上使用 `SendInput`，macOS 使用 `CGEvent`，Linux 使用 `xdotool` 的封装，模拟按下 `Cmd/Ctrl + V`。

## 3. UI/UX 风格指引
- **毛玻璃效果**：在 macOS 上开启 `window_manager` 的背景模糊效果，保持原生感。
- **无感刷新**：剪贴板更新时，后台静默入库，不产生任何 UI 抖动。
- **键盘直达**：所有功能必须可以通过键盘组合键完成，减少鼠标依赖。
- **macOS Accessibility**: 
  - 使用 `hotkey_manager` 提供的权限检查 API：`hotKeyManager.checkApiPrivilege()`。
  - 若无权限，展示引导弹窗，并提供跳转至系统设置的按钮。
- **Windows**: 
  - 默认无需特殊权限，但如果需要向“管理员身份运行”的程序粘贴，应用本身也需要以管理员身份运行。
- **沙盒配置 (macOS)**: 
  - 修改 `Debug/Release.entitlements`，确保 `com.apple.security.app-sandbox` 设为 `false`（对于剪贴板助手类工具，通常需要非沙盒模式）或添加剪贴板访问权限。

## 4. 跨平台适配方案
- **Windows**: 修改 `windows/runner/main.cpp` 实现启动瞬间隐藏窗口。
- **macOS**: 在 `Info.plist` 中配置 `LSUIElement=true` 使其成为后台应用，不出现在 Dock。
- **Linux**: 使用 `libappindicator` 适配主流桌面环境托盘。

## 4. 数据一致性与安全
- 剪贴板敏感数据不上传云端，仅本地加密存储（可选）。
- 设定数据库自动清理机制（超过最大存储条数）。
