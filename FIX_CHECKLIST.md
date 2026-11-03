# 审计修复任务清单

> **用途**：跟踪 [`AUDIT_LOGIC_REPORT.md`](AUDIT_LOGIC_REPORT.md) 中各项问题的修复进度。  
> **详情**：每条 `ID` 的完整说明、根因与修复方向，点「详情」链到审计锚点（例如 [`C-01`](AUDIT_LOGIC_REPORT.md#C-01)）。  
> **更新规则**：见 `.cursor/rules/audit-fix-checklist.mdc`。Agent / Goal 修复时必须同步改本文件。

## 状态约定

| 状态 | 含义 |
|------|------|
| `pending` | 未开始 |
| `in_progress` | 正在修 |
| `done` | 已合入并自检通过 |
| `deferred` | 有意延后（须在 Notes 写原因） |
| `wontfix` | 不修（须在 Notes 写产品/技术理由） |

## 进度摘要

| 批次 | 主题 | pending | in_progress | done | deferred | wontfix |
|------|------|---------|-------------|------|----------|---------|
| P0 | 关机 / 预览线程亲和 | 0 | 0 | 4 | 0 | 0 |
| P1 | 剪贴板回声去重 | 0 | 0 | 4 | 0 | 0 |
| P2 | 粘贴可靠性 | 0 | 0 | 7 | 0 | 0 |
| P3 | UI / 设置一致性 | 0 | 0 | 6 | 0 | 0 |
| P4 | 捕获正确性 | 0 | 0 | 4 | 0 | 0 |
| P5 | 性能与卡顿 | 0 | 0 | 3 | 0 | 0 |
| P6 | 其余 Medium | 16 | 0 | 0 | 0 | 0 |
| P7 | Low / 工程债 | 22 | 0 | 0 | 0 | 0 |

> 改条目状态后，**重算本表各列计数**（同 ID 只计一次）。

---

## 清单维护协议（给 Agent / Goal）

1. 开工前：把目标 ID 标为 `in_progress`；同一批次可并行多项，但一次 Goal 建议聚焦一批。
2. 查详情：打开 `AUDIT_LOGIC_REPORT.md#<ID>`，按「问题 / 影响 / 修复方向」实现；不要只改表象。
3. 修完：状态 → `done`，填写 `Notes`；**为该次修复创建一条 Conventional Commits 规范的 git commit**（见下方「提交规范」）；关联多 ID 可合为一次 commit，message 中列出全部 ID。
4. 不做：`deferred` / `wontfix` + Notes 原因；不要删行；不建空 commit。
5. 关联项：修 C-01 时核对 M-17；修 H-01 时核对 H-02/H-08；在 Notes 写「连带」。
6. 进度摘要表与下方条目状态保持一致。

### 提交规范（Conventional Commits）

- 每一条（或一组明确连带的）修复合入后必须 `git commit`，遵循 Conventional Commits：
  - 格式：`<type>(<scope>): <description>`
  - 常用 type：`fix`（缺陷）、`refactor`、`perf`、`test`、`docs`（仅清单/审计文档时）
  - scope 示例：`clipboard`、`paste`、`preview`、`storage`、`hotkey`、`tray`、`settings`、`ui`
  - description 用英文祈使句并带审计 ID，例如：`fix(clipboard): order-invariant paste echo fingerprint (H-01)`
- 正文可选：简要 why；`Refs: H-01, H-02`
- **每完成一项（或一组连带项）就提交一次**，不要攒成巨型 commit。
- 遵守仓库 git 安全约定：不改 git config、不 `--no-verify`、不 force push；Windows 下用 PowerShell here-string 传 message。

---

## P0 — 关机 / 预览线程亲和

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| C-01 | done | 主窗销毁前未停 PreviewWorker | [C-01](AUDIT_LOGIC_REPORT.md#C-01) | ExitApplication 在 DestroyWindow 前 Stop preview |
| C-02 | done | 预览 UI 跑在非 UI 线程 | [C-02](AUDIT_LOGIC_REPORT.md#C-02) | PreviewWindow 归 UI 线程；PreviewWorker 仅加载/解码 |
| M-17 | done | Shutdown 不停 preview（放大 C-01） | [M-17](AUDIT_LOGIC_REPORT.md#M-17) | ApplicationController::Shutdown 调用 PreviewWorker::Stop |
| H-05 | done | 异步 `[this]` 回调 vs 退出 | [H-05](AUDIT_LOGIC_REPORT.md#H-05) | m_exiting/m_shutdown 守卫；Shutdown 清空 UI 回调队列 |

## P1 — 剪贴板回声去重

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| H-01 | done | 指纹顺序 + 合成格式导致粘贴回声入库 | [H-01](AUDIT_LOGIC_REPORT.md#H-01) | 用 GetClipboardSequenceNumber 抑制自写；HashCapture 顺序无关 |
| H-02 | done | 捕获失败清除期望指纹 | [H-02](AUDIT_LOGIC_REPORT.md#H-02) | 序列号匹配即跳过，不再因 open/capture 失败清抑制态 |
| H-03 | done | SetClipboardData 半提交 | [H-03](AUDIT_LOGIC_REPORT.md#H-03) | Set 失败后 EmptyClipboard 并放弃 self-write 标记 |
| H-08 | done | 单槽期望指纹被覆盖 | [H-08](AUDIT_LOGIC_REPORT.md#H-08) | 以最新 clipboard sequence 为准抑制回声 |

## P2 — 粘贴可靠性

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| H-04 | done | 前台失败静默且剪贴板已改 | [H-04](AUDIT_LOGIC_REPORT.md#H-04) | PasteResult::Failed 时提示错误 |
| H-09 | done | 先写剪贴板后抢焦点时间窗 | [H-09](AUDIT_LOGIC_REPORT.md#H-09) | 先抢焦点再 WriteClipboard，再 SendInput |
| M-13 | done | 修饰键等待 3s 超时放弃粘贴 | [M-13](AUDIT_LOGIC_REPORT.md#M-13) | 超时仍强制发送一次 Ctrl+V |
| M-14 | done | SendInput 不检查返回值 | [M-14](AUDIT_LOGIC_REPORT.md#M-14) | 校验 SendInput 计数 |
| M-16 | done | 粘贴目标 HWND 过期/未捕获 | [M-16](AUDIT_LOGIC_REPORT.md#M-16) | 粘贴前校验/重捕获；无效则报错 |
| M-23 | done | m_pasteInProgress 过早清除 | [M-23](AUDIT_LOGIC_REPORT.md#M-23) | Deferred 粘贴结束前保持；定时器结束清除 |
| M-24 | done | Shift 被当成阻塞修饰键 | [M-24](AUDIT_LOGIC_REPORT.md#M-24) | 仅 Alt 触发等待定时器 |

## P3 — UI / 设置一致性

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| M-01 | done | Alt+托盘暂停不持久化 | [M-01](AUDIT_LOGIC_REPORT.md#M-01) | ToggleIgnoreEvents→SetIgnorePolicy 统一持久化/刷新 |
| M-02 | done | 设置窗副本覆盖托盘变更 | [M-02](AUDIT_LOGIC_REPORT.md#M-02) | SyncOpenSettingsWindow 在 ignore 变更后同步快照 |
| M-03 | done | m_loadingList 卡住挡粘贴 | [M-03](AUDIT_LOGIC_REPORT.md#M-03) | 仅最新 generation 清 loading；拒收最新结果时清 loading |
| M-12 | done | ignore 状态机分散 | [M-12](AUDIT_LOGIC_REPORT.md#M-12) | SetIgnorePolicy 单一入口；连带 M-01/M-02 |
| M-21 | done | ignore_next 无 RC 控件 | [M-21](AUDIT_LOGIC_REPORT.md#M-21) | Advanced 页补 IDC_X_IGNORE_NEXT + 本地化 |
| M-22 | done | ignore_next 依赖 ignore_events | [M-22](AUDIT_LOGIC_REPORT.md#M-22) | OnClipboardUpdate 独立 one-shot 跳过 |

## P4 — 捕获正确性

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| M-19 | done | CaptureBitmapAsDib 拒负高度 | [M-19](AUDIT_LOGIC_REPORT.md#M-19) | abs(bmHeight) 计算尺寸 |
| M-20 | done | 单格式忽略导致整次丢弃 | [M-20](AUDIT_LOGIC_REPORT.md#M-20) | 非 ViewerIgnore 格式仅跳过该格式 |
| M-25 | done | save_text 关时正则不看 HTML/RTF | [M-25](AUDIT_LOGIC_REPORT.md#M-25) | ignore 探针 + SearchPartsFromFormats |
| M-26 | done | 默认忽略格式名可能匹配失败 | [M-26](AUDIT_LOGIC_REPORT.md#M-26) | 注册名/别名/format ID 匹配 |

## P5 — 性能与卡顿

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| M-04 | done | UI 线程同步 Invoke 阻塞 | [M-04](AUDIT_LOGIC_REPORT.md#M-04) | Persist/MarkCopied/Delete/Pin 热路径异步；清历史仍同步 |
| M-10 | done | SearchHistory 全表装入内存 | [M-10](AUDIT_LOGIC_REPORT.md#M-10) | 列表 preview/search body substr 截断 |
| M-11 | done | 用户/搜索正则 ReDoS | [M-11](AUDIT_LOGIC_REPORT.md#M-11) | RegexSearchBounded 64K haystack |

## P6 — 其余 Medium

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| H-06 | pending | CallNextHookEx(nullptr) | [H-06](AUDIT_LOGIC_REPORT.md#H-06) | |
| H-07 | pending | Win 键延迟重放时序 | [H-07](AUDIT_LOGIC_REPORT.md#H-07) | |
| M-05 | pending | Storage+Preview 双 SQLite 连接 | [M-05](AUDIT_LOGIC_REPORT.md#M-05) | |
| M-06 | pending | 预览取消不回调 UI | [M-06](AUDIT_LOGIC_REPORT.md#M-06) | |
| M-07 | pending | Thumbnail COM 失败静默 | [M-07](AUDIT_LOGIC_REPORT.md#M-07) | |
| M-08 | pending | 跨线程 HBITMAP 绘制 | [M-08](AUDIT_LOGIC_REPORT.md#M-08) | |
| M-09 | pending | Settings Save：DB 与注册表不一致 | [M-09](AUDIT_LOGIC_REPORT.md#M-09) | |
| M-15 | pending | LL 钩子内 GetFocus 脆 | [M-15](AUDIT_LOGIC_REPORT.md#M-15) | |
| M-18 | pending | 全局热键安装失败无提示 | [M-18](AUDIT_LOGIC_REPORT.md#M-18) | |

> 说明：H-06/H-07 严重度为 High，但实施批次放在 P6（粘贴/回声之后），避免与 P0–P2 抢焦点；若单独开 Goal 可优先。

## P7 — Low / 工程债

| ID | Status | Summary | 详情 | Notes |
|----|--------|---------|------|-------|
| L-01 | pending | Preview reposition stale owner | [L-01](AUDIT_LOGIC_REPORT.md#L-01) | |
| L-02 | pending | StorageWorker::Stop 丢 UI 回调 | [L-02](AUDIT_LOGIC_REPORT.md#L-02) | |
| L-03 | pending | 缩放边框未 DPI 感知 | [L-03](AUDIT_LOGIC_REPORT.md#L-03) | |
| L-04 | pending | 托盘图标 mask 不正确 | [L-04](AUDIT_LOGIC_REPORT.md#L-04) | |
| L-05 | pending | UpdateChecker Start 阻塞 join | [L-05](AUDIT_LOGIC_REPORT.md#L-05) | |
| L-06 | pending | 同指纹不刷新 formats/FTS | [L-06](AUDIT_LOGIC_REPORT.md#L-06) | |
| L-07 | pending | main 异常路径依赖析构顺序 | [L-07](AUDIT_LOGIC_REPORT.md#L-07) | |
| L-08 | pending | 64-bit 指纹碰撞风险 | [L-08](AUDIT_LOGIC_REPORT.md#L-08) | |
| L-09 | pending | CF_BITMAP→DIB 调色板/位深 | [L-09](AUDIT_LOGIC_REPORT.md#L-09) | |
| L-10 | pending | FilesPreview 无总长度上限 | [L-10](AUDIT_LOGIC_REPORT.md#L-10) | |
| L-11 | pending | 白名单模式空 application 丢弃 | [L-11](AUDIT_LOGIC_REPORT.md#L-11) | |
| L-12 | pending | RegisterGlobalHotKey 名实不符 | [L-12](AUDIT_LOGIC_REPORT.md#L-12) | |
| L-13 | pending | 托盘未 NIM_SETVERSION | [L-13](AUDIT_LOGIC_REPORT.md#L-13) | |
| L-14 | pending | HasPendingKeyboardInput 窥视局限 | [L-14](AUDIT_LOGIC_REPORT.md#L-14) | |
| L-15 | pending | 自定义消息 ID 间隙 | [L-15](AUDIT_LOGIC_REPORT.md#L-15) | |
| L-16 | pending | PinKeys 集合固定 | [L-16](AUDIT_LOGIC_REPORT.md#L-16) | |
| L-17 | pending | HTML/RTF 剥离粗糙 | [L-17](AUDIT_LOGIC_REPORT.md#L-17) | |
| L-18 | pending | KeyboardHandler C 回调风格 | [L-18](AUDIT_LOGIC_REPORT.md#L-18) | |
| L-19 | pending | 测试覆盖缺口 | [L-19](AUDIT_LOGIC_REPORT.md#L-19) | |
| L-20 | pending | 页脚 Enter 无动作 | [L-20](AUDIT_LOGIC_REPORT.md#L-20) | |
| L-21 | pending | 小键盘序号粘贴 | [L-21](AUDIT_LOGIC_REPORT.md#L-21) | |
| L-22 | pending | OpenClipboard 无 NULL 回退 | [L-22](AUDIT_LOGIC_REPORT.md#L-22) | |

---

## Goal 指令示例

### 修复全部问题（主 Goal）

可直接用于创建 Goal 的 `objective`：

```text
修复 FIX_CHECKLIST.md 中全部审计问题（P0–P7，所有 pending/in_progress 条目），直到清单中每一项均为 done、deferred 或 wontfix，且进度摘要计数正确。

工作协议：
1. 单一真相源：进度与 Status/Notes 只更新 FIX_CHECKLIST.md；每项详情必须先读 AUDIT_LOGIC_REPORT.md#<ID>（问题/影响/修复方向），再改代码。遵守 .cursor/rules/audit-fix-checklist.mdc、AGENTS.md、WTL.md。
2. 按批次顺序推进：P0 → P1 → P2 → P3 → P4 → P5 → P6 → P7。一批内可按依赖合并（如 C-01+M-17；H-01+H-02+H-08）。不要跳过更高优先级批次去先做 Low，除非被阻塞且已在 Notes 标明。
3. 状态机：开始修某 ID → in_progress；合入并自检通过 → done + Notes（关键文件或行为一句话）；有意不做 → deferred/wontfix + 必须写原因。禁止删除清单行。每批开始与结束都重算进度摘要表。
4. 每完成一条修复（或一组明确连带的 ID）必须创建一个符合 Conventional Commits 的 git commit，再开始下一项：
   - 格式：<type>(<scope>): <description>
   - 优先 type=fix；scope 如 clipboard/paste/preview/storage/hotkey/tray/settings/ui
   - description 含审计 ID，例：fix(clipboard): order-invariant paste echo fingerprint (H-01)
   - 连带多项可一次 commit，message/正文列出全部 Refs ID
   - 同步把 FIX_CHECKLIST.md 的 Status/Notes/进度摘要纳入同一次 commit（或紧随的 docs 提交）
   - 不改 git config；不用 --no-verify；不 force push；不空 commit；Windows 用 PowerShell here-string 传 message
5. 根因优先、KISS/YAGNI；不引入 Qt/Electron 等新 UI 框架；能用 WTL 则用 WTL。避免无关重构。
6. 每完成一批：简短汇报完成的 ID、对应 commit hash、验证方式、本批 deferred/wontfix；然后继续下一批，直到全部收尾。
7. 全部条目非 pending/in_progress 后：通读 FIX_CHECKLIST.md 确认无遗漏，进度摘要归零 pending/in_progress，再结束 Goal。
8. 能编译则编译相关目标；关键路径说明手工验证要点（退出/粘贴回声/粘贴焦点等）。
```

发起方式：

```text
创建一个 Goal，objective 如下：
<粘贴上方「修复全部问题」整段>
```

建议同时附上：`@FIX_CHECKLIST.md` `@AUDIT_LOGIC_REPORT.md`

### 其他可选 Goal（窄范围）

**单批 P0：**

```text
按 FIX_CHECKLIST.md 只修复 P0。读 AUDIT_LOGIC_REPORT.md#ID；C-01 与 M-17 一并处理；更新 Status/进度摘要；不要扩到 P1+。
```

**单项：**

```text
修复 FIX_CHECKLIST.md 的 H-01；详情 AUDIT_LOGIC_REPORT.md#H-01；核对 H-02/H-08；完成后更新清单。
```

---

## 修订记录

| 日期 | 说明 |
|------|------|
| 2026-11-03 | 初版：P0–P7 全量条目，链到审计锚点；附 Goal 示例与维护协议 |
| 2026-11-03 | 全量 Goal 要求每条修复创建 Conventional Commits git commit |
