# 第三方内容说明

本目录记录随源码或二进制发行包分发的第三方代码、图标和视觉资源。各组件仍受其原始许可证约束。

## Maccy 视觉资源

- 来源项目：[p0deje/Maccy](https://github.com/p0deje/Maccy)
- 许可证：MIT License
- 版权声明：`Copyright (c) 2025 Alex Rodionov`
- 完整文本：[`Maccy-LICENSE.txt`](Maccy-LICENSE.txt)

本项目不是 Maccy 官方 Windows 版本，也不获得 Maccy 商标或品牌授权。分发源码或二进制文件时，请保留上述归属说明。

## 构建依赖许可证

WTL、SQLite、CPR、nlohmann-json 及其传递依赖由根目录 `vcpkg.json` 管理。许可证文本不保存在源码仓库中；执行 CMake 安装或打包时，会从 vcpkg 已安装 port 的 `share/<port>/copyright` 复制到发行目录：

- `licenses/cpr/copyright`
- `licenses/curl/copyright`
- `licenses/nlohmann-json/copyright`
- `licenses/wtl/copyright`
- `licenses/sqlite3/copyright`
- `licenses/zlib/copyright`

依赖版本和 SQLite 的 FTS5 构建功能由 `vcpkg.json` 中的 baseline 和 feature 声明确定。
