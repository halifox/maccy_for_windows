# maccy

一个使用 C++20、CMake、WTL 和 SQLite 实现的低占用 Windows 剪贴板历史工具。

## 已实现

- 常驻 Windows 通知区域，支持全局呼出热键。
- 文本、HTML、RTF、文件（`CF_HDROP`）和 DIB/DIBV5 图片的剪贴板历史保存与恢复。
- 精确、模糊、正则和混合搜索；置顶、删除、预览、复制/粘贴、去除格式。
- maccy 2.7.1 设置页对应的通用、外观、存储、忽略、置顶和高级功能。
- SQLite WAL 保存轻量历史元数据、完整剪贴板格式和全文索引；正文、图片和文件格式按需加载。

完整实现范围、构建结果以及 Windows 平台差异见 [TASK_REPORT.md](TASK_REPORT.md)。

## 构建

在 Visual Studio Developer PowerShell 或已加载 MSVC 环境的终端中执行：

```powershell
cmake --build .\cmake-build-debug --config Debug --parallel 2
cmake --build .\cmake-build-release --config Release --parallel 2
```

生成的程序为 `maccy.exe`。数据库默认保存到：

```text
%LOCALAPPDATA%\maccy\maccy.db
```

完整剪贴板格式保存在 `maccy.db` 的 `clipboard_formats` 表中，图片也以 BLOB 保存；`history_fts` 仅索引正文和文件路径，不索引图片。
