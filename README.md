# Maccy for Windows

一个 Windows 剪贴板历史工具，受 macOS 版 [Maccy](https://github.com/p0deje/Maccy) 启发。

> 本项目不是 Maccy 官方 Windows 版本，也不隶属于、代表或获得 Maccy 官方项目授权。

## 功能概览

- 通过 Windows 系统托盘和全局快捷键访问剪贴板历史；
- 保存文本、图片和文件剪贴板格式；
- 支持精确、模糊、正则和混合搜索；
- 精确搜索使用 SQLite FTS5，图片本身不参与文本搜索；
- 支持固定项目、编辑固定文本、忽略应用程序、剪贴板格式和正则内容；
- 支持 Windows 剪贴板历史记录标记，避免保存明确标记为不应进入历史的内容；
- 本地 SQLite 存储，不上传剪贴板内容。

## 与原版 Maccy 的差异

- Maccy 使用 macOS 菜单栏；本项目使用 Windows 系统托盘和全局快捷键。
- Maccy 定时检查剪贴板；本项目使用 Windows `WM_CLIPBOARDUPDATE` 事件监听。
- Maccy 主要搜索生成后的项目标题；本项目还会搜索保存的文本正文和文件路径。
- Maccy 会尝试使用 OCR 生成图片标题；本项目不进行 OCR，图片仅作为图片项目保存和预览。
- Maccy 使用 macOS Pasteboard 和 SwiftData；本项目使用 Windows 原生剪贴板格式和 SQLite BLOB。
- 本项目不包含 Universal Clipboard、iCloud 或 macOS App Intents 等 macOS 集成。

## 下载和安装

请从 [GitHub Releases](https://github.com/halifox/Clipboard/releases) 下载带有版本号的 ZIP 或 NSIS 安装包，并核对同一 Release 中的 `SHA256SUMS.txt`。本地 Debug 构建目录中的 `maccy.exe` 仅适用于开发和测试，不是正式分发包。

用户可见变化记录见 [`CHANGELOG.md`](CHANGELOG.md)。

## 从源码构建

开发环境要求：

- Windows x64；
- Visual Studio C++ 工具链；
- CMake 3.25 或更高版本；
- Ninja；
- C++20 编译器。

请在 Visual Studio Developer PowerShell 中执行：

```powershell
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release --output-on-failure
```

启动构建结果：

```powershell
Start-Process .\build\windows-x64-release\maccy.exe
```

生成便携 ZIP：

```powershell
cpack --config .\build\windows-x64-release\CPackConfig.cmake -G ZIP
```

如果使用普通 PowerShell，请先加载 Visual Studio 的 C++ 开发环境。本文档提供的构建配置针对 Windows x64。

## 数据、隐私和删除

剪贴板历史包含用户可能不希望长期保存的密码、令牌、私人文本、图片和文件路径。应用默认将数据保存在：

```text
%LOCALAPPDATA%\maccy\maccy.db
```

SQLite 的 `-wal` 和 `-shm` 文件也是数据库的一部分。当前存储未提供应用层加密；拥有当前 Windows 用户数据访问权限的进程可能读取这些内容。请在使用密码管理器、银行信息或其他敏感数据时配置忽略规则，并阅读 [隐私和数据说明](PRIVACY.md)。

应用不包含遥测、剪贴板上传或后台自动下载安装服务。更新入口仅通过浏览器打开项目的 GitHub Releases 页面。

删除历史并不等同于取证意义上的安全擦除。需要彻底清理时，应退出应用后按照 [隐私和数据说明](PRIVACY.md) 删除数据库及其 WAL 文件。

## 已知限制

- 当前没有完整的自动化 CTest 测试套件；
- 目前提供的构建配置和预构建包面向 Windows x64，ARM64 和 32 位版本不在支持范围内；
- 应用不提供自动下载和安装更新，请手动从 GitHub Releases 获取新版本；
- 升级前请阅读对应版本的变更说明，并备份需要保留的历史记录。

## 许可证和第三方内容

除明确标注的第三方文件外，本项目源代码采用 MIT License，见 [`LICENSE`](LICENSE)。

第三方组件和视觉资源遵循各自的许可证，来源和分发说明见 [`THIRD_PARTY_NOTICES/README.md`](THIRD_PARTY_NOTICES/README.md)：

- Maccy 图标和视觉资源：MIT License，见 [`THIRD_PARTY_NOTICES/Maccy-LICENSE.txt`](THIRD_PARTY_NOTICES/Maccy-LICENSE.txt)；
- WTL 头文件：Microsoft Public License，见 [`THIRD_PARTY_NOTICES/WTL-MS-PL.txt`](THIRD_PARTY_NOTICES/WTL-MS-PL.txt)；
- SQLite：Public Domain，见 [`THIRD_PARTY_NOTICES/SQLite-PUBLIC-DOMAIN.txt`](THIRD_PARTY_NOTICES/SQLite-PUBLIC-DOMAIN.txt)。

Maccy 的名称和 Logo 仍属于其相应权利人；本项目的许可证声明不构成商标或品牌授权。

## 参与贡献

欢迎提交问题、改进建议和 Pull Request。提交前请阅读：

- [贡献指南](CONTRIBUTING.md)；
- [安全政策](SECURITY.md)。

请不要在公开 Issue 中提交安全漏洞、密码、令牌或未脱敏的剪贴板内容。

## 致谢

感谢 [Alex Rodionov](https://github.com/p0deje) 和 Maccy 项目为 macOS 提供了优秀的剪贴板工具，也感谢其 MIT 许可证允许社区学习、修改和再利用相关软件与资源。
