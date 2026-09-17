# Maccy for Windows

一个独立的 Windows 剪贴板历史工具，受 macOS 版 [Maccy](https://github.com/p0deje/Maccy) 启发。

> 本项目是独立开发的 Windows 项目，不是 Maccy 官方 Windows 版本，也不隶属于、代表或获得 Maccy 官方项目授权。

## 初衷

我同时使用 Windows 和 macOS 设备。

macOS 上的 Maccy 非常好用：极简、高效的界面让剪贴板历史随时可用，也确实提升了我的日常生产力。反观 Windows 平台下的许多剪贴板工具，功能越来越多，却往往与“高效”相距甚远。

因此，我决定自己动手开发一个 Windows 版本的剪贴板历史工具，保留简洁的交互，并将低常驻内存作为核心设计目标。

## 当前方向

- 快速呼出剪贴板历史并搜索内容
- 支持系统托盘运行和快捷键操作
- 支持置顶、删除、历史记录管理等常用操作
- 使用原生 Windows UI、WTL、C++20 和 SQLite 构建
- 按需加载剪贴板数据，减少长期运行时的内存占用

## 许可证

除明确标注的第三方文件外，本项目源代码采用 MIT License。
第三方组件和视觉资源分别遵循其各自的许可证。

项目自身源代码的许可证见 [`LICENSE`](LICENSE)。

当前仓库中的第三方内容包括：

- Maccy 图标和视觉资源：MIT License，见 [`THIRD_PARTY_NOTICES/Maccy-LICENSE.txt`](THIRD_PARTY_NOTICES/Maccy-LICENSE.txt)
- WTL 头文件：Microsoft Public License，见 [`THIRD_PARTY_NOTICES/WTL-MS-PL.txt`](THIRD_PARTY_NOTICES/WTL-MS-PL.txt)
- SQLite：Public Domain，详见 [SQLite 官方说明](https://www.sqlite.org/copyright.html)

## 构建

需要：

- Windows 10/11
- Visual Studio C++ 桌面开发工具和 Windows SDK
- CMake
- Ninja（使用 Ninja 生成器时）

在已加载 Visual Studio 开发环境的终端中执行：

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix dist
```

## 与 Maccy 的关系

本项目参考了 Maccy 的产品理念、部分交互和视觉资源，但代码和 Windows 实现是独立开发的。本项目不代表 Maccy 官方，也不应被视为官方移植版本。

项目中使用的部分 Maccy 图标和视觉资源遵循 Maccy 的 MIT 许可证。版权归属：

```text
Copyright (c) 2025 Alex Rodionov
```

完整许可证文本见 [`THIRD_PARTY_NOTICES/Maccy-LICENSE.txt`](THIRD_PARTY_NOTICES/Maccy-LICENSE.txt)。Maccy 的名称和 Logo 仍属于其相应权利人；本项目的许可证声明不构成商标或品牌授权。

## 致谢

感谢 [Alex Rodionov](https://github.com/p0deje) 和 Maccy 项目为 macOS 提供了优秀的剪贴板工具，也感谢其 MIT 许可证允许社区学习、修改和再利用相关软件与资源。
