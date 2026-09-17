# Maccy for Windows

一个 Windows 剪贴板历史工具，受 macOS 版 [Maccy](https://github.com/p0deje/Maccy) 启发。

> 本项目不是 Maccy 官方 Windows 版本，也不隶属于、代表或获得 Maccy 官方项目授权。

## 初衷

我同时使用 Windows 和 macOS 设备。

macOS 上的 Maccy 非常好用：极简、高效的界面让剪贴板历史随时可用，也确实提升了我的日常生产力。反观 Windows 平台下的许多剪贴板工具，功能越来越多，却往往与“高效”相距甚远。

因此，我决定自己动手开发一个 Windows 版本的剪贴板历史工具，保留简洁的交互，并将低常驻内存作为核心设计目标。

## 与原版 Maccy 的差异

- Maccy 使用 macOS 菜单栏；本项目使用 Windows 系统托盘和全局快捷键。
- Maccy 可以在菜单栏图标旁显示最近复制的内容；Windows 托盘无法提供同样的内联文本展示，因此该选项在本项目中不可用。
- Maccy 定时检查剪贴板；本项目使用 Windows `WM_CLIPBOARDUPDATE` 事件监听。
- Maccy 主要搜索生成后的项目标题；本项目还会搜索保存的文本正文和文件路径。支持精确、模糊、正则和混合模式，精确搜索使用 FTS5 加速，图片本身不参与搜索。
- Maccy 会尝试使用 OCR 生成图片标题；本项目不进行 OCR，图片仅作为图片项目保存和预览。
- Maccy 使用 macOS Pasteboard 和 SwiftData；本项目使用 Windows 原生剪贴板格式和 SQLite BLOB。
- Maccy 按 Bundle ID 和 Pasteboard 类型忽略；本项目按 Windows 应用程序路径/文件名和剪贴板格式名忽略，并支持正则表达式。
- 本项目不包含 Universal Clipboard、iCloud 或 macOS App Intents 等 macOS 集成。
- 本项目新增“遵循 Windows 剪贴板历史记录标记”选项，来源明确标记为不应进入历史的内容将不会被记录。

## 许可证

除明确标注的第三方文件外，本项目源代码采用 MIT License。
第三方组件和视觉资源分别遵循其各自的许可证。

项目自身源代码的许可证见 [`LICENSE`](LICENSE)。

当前仓库中的第三方内容包括：

- Maccy 图标和视觉资源：MIT License，见 [`THIRD_PARTY_NOTICES/Maccy-LICENSE.txt`](THIRD_PARTY_NOTICES/Maccy-LICENSE.txt)
- WTL 头文件：Microsoft Public License，见 [`THIRD_PARTY_NOTICES/WTL-MS-PL.txt`](THIRD_PARTY_NOTICES/WTL-MS-PL.txt)
- SQLite：Public Domain，详见 [SQLite 官方说明](https://www.sqlite.org/copyright.html)

Maccy 图标和视觉资源的版权归属为：

```text
Copyright (c) 2025 Alex Rodionov
```

Maccy 的名称和 Logo 仍属于其相应权利人；本项目的许可证声明不构成商标或品牌授权。

## 致谢

感谢 [Alex Rodionov](https://github.com/p0deje) 和 Maccy 项目为 macOS 提供了优秀的剪贴板工具，也感谢其 MIT 许可证允许社区学习、修改和再利用相关软件与资源。
