# 任务开发计划 (Roadmap & Tasks)

## 阶段一：环境搭建与基础架构 (Priority: High)
- [ ] 1.1 更新 `pubspec.yaml` 添加核心依赖插件（Riverpod, Drift, Hooks, GoRouter 等）。
- [ ] 1.2 配置各平台原生设置（Windows 隐藏、macOS `Info.plist` 与权限）。
- [ ] 1.3 运行 `build_runner` 生成初步的 Riverpod 与 Drift 代码。
- [ ] 1.4 初始化 `AppWindowManager` 与 `GoRouter` 路由配置。
- [ ] 1.5 实现权限检查逻辑：特别是 macOS 的辅助功能权限。

## 阶段三：数据持久化 (Priority: Medium)
- [ ] 3.1 配置 `Drift` 数据库表结构与 DAO。
- [ ] 3.2 实现剪贴板内容的去重、存储与 Riverpod 状态同步。

## 阶段四：UI 开发 (Maccy Fidelity)
- [ ] 4.1 历史页面开发：实现无边框、跟随光标的弹出逻辑。
- [ ] 4.2 键盘驱动交互：实现 `0-9` 数字快捷键选择、`Up/Down` 导航及模糊搜索。
- [ ] 4.3 搜索算法集成：确保搜索响应在 10ms 以内。
- [ ] 4.4 设置页面：仿 Maccy 风格的简洁配置项。
- [ ] 4.5 视觉美化：macOS 模糊背景与 Windows Fluent 设计。

## 阶段五：进阶功能 (Priority: Low)
- [ ] 5.1 模拟自动粘贴功能 (Auto-paste via key simulation)。
- [ ] 5.2 支持图片/富文本预览。
- [ ] 5.3 历史记录持久化优化与加密。

---
**当前状态**: 规划完成，等待开发启动。
