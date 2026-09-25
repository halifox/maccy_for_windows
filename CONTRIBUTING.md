# 贡献指南

感谢你对 Maccy for Windows 的关注。

本项目是一个独立的 Windows 剪贴板历史工具，受到 macOS 版 Maccy 启发，但不是 Maccy 官方 Windows 版本，也不代表 Maccy 官方项目。

## 开始之前

提交 Issue 或 Pull Request 之前，请先：

1. 搜索已有 Issue，避免重复报告。
2. 阅读项目的 README、LICENSE 和 SECURITY.md。
3. 不要在公开内容中粘贴真实的剪贴板历史、密码、令牌、个人文件路径或数据库文件。
4. 如果变更涉及数据结构、线程模型、窗口生命周期、剪贴板行为或用户数据格式，建议先提交 Issue 讨论方案。

安全漏洞请按照 SECURITY.md 报告，不要公开提交。

## 可以贡献的内容

欢迎以下类型的贡献：

- 可稳定复现的错误修复；
- 构建、测试和调试流程改进；
- 剪贴板、搜索、数据库、预览和托盘行为改进；
- 性能、内存占用和稳定性改进；
- 文档、错误提示和本地化改进；
- 有明确使用场景且范围清晰的新功能；
- 不包含敏感数据的复现工具和测试用例。

贡献应尽量保持单一目的。请避免在同一个 Pull Request 中混入无关的重命名、格式化、目录调整或临时代码清理。

## Issue 要求

错误报告应尽可能包含：

- 应用版本或 Git commit；
- Windows 版本和系统架构；
- 使用安装包还是本地编译版本；
- 清晰的复现步骤；
- 实际结果和预期结果；
- 复现频率；
- 必要时提供经过脱敏的日志、截图或录屏；
- 如果问题与剪贴板、搜索、数据库或设置有关，请说明相关配置。

提交前请删除：

- 剪贴板正文；
- 密码、令牌和密钥；
- 真实文件内容；
- 真实用户目录；
- 未脱敏的数据库文件；
- 包含个人信息的截图和日志。

## 开发环境

项目当前使用：

- Windows；
- CMake 3.25 或更高版本；
- C++20；
- Visual Studio C++ 工具链；
- WTL；
- SQLite；
- vcpkg；
- Ninja 或其他可用的 CMake 构建器。

推荐在 Windows 的 Visual Studio Developer PowerShell 中设置 `VCPKG_ROOT`，再使用仓库提供的 CMake Preset：

```powershell
$env:VCPKG_ROOT = 'C:/path/to/vcpkg'
cmake --preset windows-x64-debug
cmake --build --preset windows-x64-debug
ctest --preset windows-x64-debug --output-on-failure
```

Release 构建和打包验证：

```powershell
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
ctest --preset windows-x64-release --output-on-failure
cpack --config .\build\windows-x64-release-vcpkg\CPackConfig.cmake -G NSIS
```

首次配置会根据根目录的 `vcpkg.json` 安装 WTL 和启用了 FTS5 的 SQLite。Preset 使用 `x64-windows-static-md` triplet，并将构建文件写入独立的 `build/<preset>-vcpkg` 目录。

如果使用其他 CMake 生成器、Visual Studio 版本、配置或架构，请在 Pull Request 中说明。发布版本必须使用干净的 Release 构建目录，不得直接分发 Debug 构建结果。

不要提交构建目录、生成的可执行文件、调试符号、安装包、数据库文件或本地配置文件。仓库的 `.gitignore` 已覆盖常见输出，但提交前仍需检查 `git status`。

## 测试

当前项目没有完整的自动化 CTest 测试套件。修改后至少应完成与变更相关的手动验证。

CTest 当前主要验证构建流程；在新增测试之前，`ctest` 通过不代表剪贴板、数据库或窗口生命周期已经得到完整覆盖。

涉及以下内容时，应额外验证：

- 托盘图标和窗口关闭行为；
- 全局快捷键；
- 搜索、筛选和排序；
- 固定、删除、清空和容量限制；
- 文本、图片和文件剪贴板格式；
- 重启后的历史和设置持久化；
- 高 DPI 和不同窗口位置；
- 长文本、大图片和快速连续复制；
- 数据库错误或占用时的错误处理。

## 代码和架构约定

- 使用 C++20 和项目现有的命名、所有权和错误处理风格。
- 固定对话框布局、静态控件文本和菜单优先放在 `.rc` 资源中；运行时状态和动态布局放在 C++ 中。
- UI 线程不应执行不可预测的数据库、BLOB 或重型预览工作。
- SQLite 写入应通过现有的存储工作线程和事务模型完成。
- 不要绕过 StorageWorker 直接从多个线程共享 SQLite 连接。
- 修改历史数据时，应同时维护格式数据、搜索索引、固定状态和删除规则的一致性。
- 只有在确实需要时才加载剪贴板 BLOB，避免无意义地读取大对象。
- 异步搜索和预览必须正确处理取消、过期结果和窗口销毁。
- 窗口隐藏、托盘驻留和显式退出是不同的生命周期行为，不要混为一谈。
- 不要通过复制逻辑、临时开关、死代码或特殊分支掩盖根因。
- 如果变更数据库结构、持久化格式或升级行为，应在 Pull Request 中明确说明兼容性和数据影响。

## 第三方代码和资源

不得提交来源不明或许可证不兼容的代码、字体、图像、图标或其他资源。

新增第三方内容时，请同时说明：

- 来源；
- 版本或 commit；
- 原始许可证；
- 是否需要保留版权声明；
- 是否需要更新 THIRD_PARTY_NOTICES。

不得暗示本项目获得 Maccy 官方商标、Logo 或项目授权。

## Pull Request

Pull Request 应包含：

- 变更目的；
- 实现方式；
- 影响范围；
- 潜在风险；
- 已完成的构建和测试；
- UI 变更的截图或录屏；
- 数据库、存储或迁移变更的说明；
- 第三方内容和许可证变更说明。

PR 标题和 PR 中每个提交的标题都必须使用 Conventional Commits 格式。CI 会检查两者；合并时请使用 squash，并保留已通过检查的 PR 标题作为 squash 提交标题。

提交 Pull Request 不代表一定会被合并。维护者可能要求缩小范围、重写实现、补充测试，或因项目方向不符而关闭变更。

## 自动发布和提交信息

发布版本由 Release Please 管理。PR 合并到 `master` 后，只有 `feat`、`fix` 或带破坏性变更标记的提交会产生版本更新 PR：

| 提交类型 | 版本变化 | 示例 |
| --- | --- | --- |
| `fix` | Patch | `fix(search): correct empty-result recovery` |
| `feat` | Minor | `feat(search): add history filters` |
| 任意类型的 `!` 或 `BREAKING CHANGE:` footer | Major | `feat!: replace the storage format` |
| `docs`、`ci`、`chore`、`build`、`test`、`refactor`、`style`、`perf` | 不单独发版 | `docs(guide): clarify setup` |

普通 PR 的标题和每个提交标题都需要遵循该格式。破坏性变更可在类型后使用 `!`，或在提交正文中添加 `BREAKING CHANGE: ...`。不要把 `BREAKING CHANGE:` 加到只包含文档、CI 或维护工作的提交中，除非该变更确实影响用户兼容性。

Release Please 在 `master` 上更新 `version.txt`、`vcpkg.json` 和 `CHANGELOG.md`，并创建版本 PR。版本 PR 仅在 `Windows CI` 的 Conventional Commits、Debug/Release x64 和 Debug/Release x86 检查全部成功后启用 squash 自动合并。合并后 Release Please 创建 `vX.Y.Z` Tag 和草稿 Release；Tag 工作流构建、测试、生成 NSIS 安装器和 SHA-256 文件，校验资产上传完成后才发布 Release。文档、CI 或维护类提交不会单独创建版本 PR。

维护者需在仓库中配置 GitHub App，并将 App Client ID 保存为 Actions 变量 `RELEASE_PLEASE_APP_CLIENT_ID`，将私钥保存为 Actions 密钥 `RELEASE_PLEASE_APP_PRIVATE_KEY`。App 安装到本仓库，并授予 Contents、Issues、Pull requests 写权限。私钥只保存在 GitHub Actions 密钥中，不要提交到仓库或公开分享。

在 `master` 分支规则中保留现有保护，并将 `Conventional Commits`、`Debug x64`、`Release x64`、`Debug x86`、`Release x86` 设为必需检查；在仓库设置中启用 Auto-merge。GitHub 自动合并会遵守审批规则，因此若当前规则要求人工审批，版本 PR 会等待审批。

如果要让版本 PR 无需人工审批，同时保留普通 PR 的审批和必需 CI，可将审批要求单独放入一个只包含审批规则的 branch ruleset，将自动化 App 仅加入该 ruleset 的 pull-request-only bypass；必需状态检查继续留在没有该 App bypass 的 branch protection 或另一个 ruleset 中。若当前使用 classic branch protection 的必需审批，需先把审批要求迁移到这个独立 ruleset。不要让 App bypass 包含 CI 检查的规则集；如果仓库设置无法把审批和检查分开，就保留现有保护并接受版本 PR 需要人工审批。

## 发布

版本 Tag、草稿 Release、构建、测试、安装器和 SHA-256 上传及 Release 发布由上述流程自动完成。仍需按 [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) 完成发布后的人工安装和运行检查。至少需要确认：

- Release x64 构建成功；
- CTest 和手动 Smoke Test 已完成；
- NSIS 安装包可以在干净环境安装并启动；
- 包含项目许可证和所有第三方许可证；
- `version.txt`、版本 Tag、CMake 版本、安装包文件名和应用 About 版本一致；
- Release 说明包含已知限制、数据影响和 SHA-256 校验和。

## 贡献许可

提交贡献时，你确认：

1. 你有权提交该贡献；
2. 该贡献不会故意包含未经许可的第三方内容；
3. 你同意该贡献可以按照本仓库的 MIT License 发布；
4. 你保留自己对贡献内容的著作权；
5. 如果贡献受到雇主或其他主体约束，你已经获得必要授权。

本文件不替代 LICENSE，也不会改变第三方组件各自的许可证。
