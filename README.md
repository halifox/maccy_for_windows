<div align="center">

<img src="src/assets/app/maccy.png" alt="Maccy for Windows" width="120" height="120" />

# Maccy for Windows

一个 Windows 剪贴板历史工具，受 macOS 版 [Maccy](https://github.com/p0deje/Maccy) 启发。

[![Platform: Windows x64](https://img.shields.io/badge/platform-Windows%20x64-0078D4)](#从源码构建)
[![Built with C++20](https://img.shields.io/badge/built%20with-C%2B%2B20-00599C)](#从源码构建)
[![Storage: SQLite](https://img.shields.io/badge/storage-SQLite-003B57)](#数据隐私和删除)
[![License: MIT](https://img.shields.io/badge/license-MIT-lightgrey)](LICENSE)
[![No telemetry](https://img.shields.io/badge/telemetry-none-brightgreen)](#数据隐私和删除)

[功能](#功能概览) · [安装](#安装) · [构建](#从源码构建) · [数据与隐私](#数据隐私和删除)

</div>

---

> [!WARNING]
>
> **本项目不是 Maccy 官方 Windows 版本，也不隶属于、代表或获得 Maccy 官方项目授权。**

## 功能概览

- 通过 Windows 系统托盘和全局快捷键访问剪贴板历史；
- 保存文本、图片和文件剪贴板格式；
- 支持精确、模糊、正则和混合搜索；
- 精确搜索使用 SQLite FTS5，图片本身不参与文本搜索；
- 支持固定项目、编辑固定文本、忽略应用程序、剪贴板格式和正则内容；
- 支持 Windows 剪贴板历史记录标记，避免保存明确标记为不应进入历史的内容；
- 可手动或在启动时检查 GitHub Releases，只提示新版本，不自动下载或替换程序；
- 本地 SQLite 存储，不上传剪贴板内容。

## 与原版 Maccy 的差异

- Maccy 使用 macOS 菜单栏；本项目使用 Windows 系统托盘和全局快捷键。
- Maccy 定时检查剪贴板；本项目使用 Windows `WM_CLIPBOARDUPDATE` 事件监听。
- Maccy 主要搜索生成后的项目标题；本项目还会搜索保存的文本正文和文件路径。
- Maccy 会尝试使用 OCR 生成图片标题；本项目不进行 OCR，图片仅作为图片项目保存和预览。
- Maccy 使用 macOS Pasteboard 和 SwiftData；本项目使用 Windows 原生剪贴板格式和 SQLite BLOB。
- 本项目不包含 Universal Clipboard、iCloud 或 macOS App Intents 等 macOS 集成。

## 内存测试

测试软件版本:1.0.1

|    历史记录 | 内容                 |   后台内存占用 |
|--------:|--------------------|---------:|
|     0 条 | 全新启动，无历史记录         |  3.07 MB |
|   200 条 | 每条 55 字节的短文本       |  3.99 MB |
|   200 条 | 每条 50 KB 的长文本      | 10.84 MB |
|   200 条 | 每张 800 × 600 像素的图片 | 11.54 MB |
|   200 条 | 短文本、长文本和图片混合       | 12.04 MB |
|  1000 条 | 每条 55 字节的短文本       |  8.38 MB |
|  1000 条 | 每条 50 KB 的长文本      | 36.05 MB |
|  1000 条 | 每张 800 × 600 像素的图片 | 14.43 MB |
|  1000 条 | 短文本、长文本和图片混合       | 18.46 MB |

## 安装

请从 [GitHub Releases](https://github.com/halifox/maccy_for_windows/releases) 下载带有版本号的 NSIS 安装程序，并核对同一 Release 中的 `SHA256SUMS.txt`。

NSIS 安装程序仅为当前 Windows 用户安装到 `%LOCALAPPDATA%\Programs\maccy`，无需管理员权限；开始菜单快捷方式和登录启动项也仅属于当前用户。安装完成页可选择启动 Maccy，桌面快捷方式可在安装过程中选择。

升级请使用同一个 NSIS 安装程序：从应用的更新提示打开 GitHub Releases，下载新版安装程序并运行。安装程序会关闭正在运行的 Maccy，在原用户安装目录内替换程序文件，并保留剪贴板历史、设置和登录启动项。旧版全机安装需先由管理员卸载一次，再安装每用户版本。

卸载时默认保留 `%LOCALAPPDATA%\maccy` 中的剪贴板历史和设置。若要同时删除历史、固定项目、设置及其他 Maccy 本地数据，请在卸载页勾选删除选项；此操作不可撤销。

用户可见变化记录见 [`CHANGELOG.md`](CHANGELOG.md)。

## 从源码构建

开发环境要求：

- Windows x64；
- Visual Studio C++ 工具链；
- CMake 3.25 或更高版本；
- Ninja；
- vcpkg；
- C++20 编译器；
- NSIS，并确保 `makensis.exe` 在 `PATH` 中。

在 Visual Studio x64 Developer PowerShell 的仓库根目录运行下面这一条命令，即可完成 Release 配置、编译，并生成 NSIS 安装程序和 SHA-256 校验文件：

```powershell
cmake -DVCPKG_ROOT=C:/dev/vcpkg -P cmake/package-x64.cmake
```

`VCPKG_ROOT` 指向 vcpkg 根目录，默认版本从根目录的 `version.txt` 读取。需要临时覆盖时可传入 `-DMACCY_VERSION="X.Y.Z"`。首次运行会通过 `vcpkg.json` 安装依赖；产物和校验文件写入 `build/packages/`。脚本使用 x64 Visual Studio Release 配置，版本号会写入程序和安装包。

## 数据、隐私和删除

剪贴板历史包含用户可能不希望长期保存的密码、令牌、私人文本、图片和文件路径。应用默认将数据保存在：

```text
%LOCALAPPDATA%\maccy\maccy.db
```

SQLite 的 `-wal` 和 `-shm` 文件也是数据库的一部分。当前存储未提供应用层加密；拥有当前 Windows 用户数据访问权限的进程可能读取这些内容。请在使用密码管理器、银行信息或其他敏感数据时配置忽略规则，并阅读 [隐私和数据说明](PRIVACY.md)。

应用不包含遥测、剪贴板上传或后台自动下载安装服务。更新检查只请求 GitHub Releases 的公开版本信息；发现新版本后，应用会在用户确认后打开浏览器，不会自动下载或替换程序。

删除历史并不等同于取证意义上的安全擦除。需要彻底清理时，应退出应用后按照 [隐私和数据说明](PRIVACY.md) 删除数据库及其 WAL 文件。

## 许可证和第三方内容

除明确标注的第三方文件外，本项目源代码采用 MIT License，见 [`LICENSE`](LICENSE)。

第三方组件和视觉资源遵循各自的许可证，来源和分发说明见 [`THIRD_PARTY_NOTICES/README.md`](THIRD_PARTY_NOTICES/README.md)：

- Maccy 图标和视觉资源：MIT License，见 [`THIRD_PARTY_NOTICES/Maccy-LICENSE.txt`](THIRD_PARTY_NOTICES/Maccy-LICENSE.txt)；
- WTL、SQLite、CPR、nlohmann-json 及其传递依赖：许可证和版权声明见发行包 `licenses/<port>/copyright` 目录，具体文件位置见第三方内容说明。

Maccy 的名称和 Logo 仍属于其相应权利人；本项目的许可证声明不构成商标或品牌授权。

## 参与贡献

欢迎提交问题、改进建议和 Pull Request。提交前请阅读：

- [贡献指南](CONTRIBUTING.md)；
- [安全政策](SECURITY.md)。

请不要在公开 Issue 中提交安全漏洞、密码、令牌或未脱敏的剪贴板内容。

## 致谢

感谢 [Alex Rodionov](https://github.com/p0deje) 和 Maccy 项目为 macOS 提供了优秀的剪贴板工具，也感谢其 MIT 许可证允许社区学习、修改和再利用相关软件与资源。

