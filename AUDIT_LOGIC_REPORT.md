# Maccy for Windows — 逻辑审计报告

- **审计对象**：`C:\Users\user\CLionProjects\maccy`（Windows 剪贴板历史 / 托盘应用）
- **审计视角**：Windows 原生开发（Win32 / WTL / 剪贴板 / 低级键盘钩子 / 多线程 / SQLite）
- **审计方式**：静态代码审查（未跑集成测试 / 未做运行时复现）
- **范围**：`src/` 核心业务与窗口逻辑（不含打包脚本细节与文档措辞）
- **修复跟踪**：状态与批次见 [`FIX_CHECKLIST.md`](FIX_CHECKLIST.md)；本文件各条目前有稳定锚点（`#C-01`、`#H-01` …）供清单回链

---

## 1. 总览

项目整体架构清晰：UI 线程 + `StorageWorker` 单写连接 + `PreviewWorker` / `ThumbnailWorker` / `UpdateChecker` 后台线程，剪贴板监听走 `AddClipboardFormatListener`，打开弹窗热键走 `WH_KEYBOARD_LL`。

主要风险集中在五类：

1. **窗口/线程生命周期**（预览窗体跨线程、退出顺序）
2. **剪贴板自写回声（echo）去重**（指纹与系统合成格式）
3. **粘贴焦点恢复与 `SendInput`**
4. **异步回调捕获 `this` 与 UI 阻塞 `Invoke`**
5. **设置状态多副本 / 持久化不一致**

下文按严重级别列出全部发现问题。编号全局唯一，便于跟踪。

**严重级别说明**

| 级别 | 含义 |
|------|------|
| Critical | 可导致崩溃、挂起、明显数据损坏或核心功能系统性失败 |
| High | 高频用户路径错误、历史污染、粘贴失败、显著竞态 |
| Medium | 边界条件/性能/一致性问题，可复现或影响体验 |
| Low | 工程债、DPI/视觉、边缘路径、可维护性问题 |

---

## 2. Critical

<a id="C-01"></a>
### C-01 主窗口销毁时预览工作线程尚未停止

- **位置**：`MainWindow::ExitApplication`（约 1468–1485）；`main.cpp` 消息循环退出后才 `preview.Stop()`（约 358–362）；`ApplicationController::Shutdown` 不停止 preview
- **问题**：`PreviewWindow` 在 `PreviewWorker` 线程上创建并泵消息；退出时 UI 线程先 `DestroyWindow()` 销毁 owner，preview 的 `Stop()` 要等消息循环结束后才执行。
- **影响**：跨线程 owner/owned 窗口销毁是典型 Win32 生命周期坑——退出阶段可能挂起、随机访问违例、预览窗残留。
- **修复方向**：在 `DestroyWindow()` 之前同步 `m_previewWorker.Stop()`（安装器退出 / 初始化失败路径同样处理）；或让 `ApplicationController::Shutdown` 负责停 preview。

<a id="C-02"></a>
### C-02 预览 UI 完全跑在非 UI 线程

- **位置**：`PreviewWorker::ThreadMain` / `ProcessRequest`；`PreviewWindow` 的创建、`SetWindowPos`、`ShowWindow`、绘制
- **问题**：HWND 创建、显示、GDI 绘制均在 worker 线程，而非拥有 `MainWindow` 的 UI 线程。
- **影响**：违反 UI 线程亲和；与主线程的激活、焦点、DPI、`ContainsWindow`、销毁路径交织时出现难以复现的故障。
- **修复方向**：解码/读库留在后台；通过 `PostMessage` 把结果送回主线程，在主线程创建/显示 owned 预览窗；或预览仅主线程、解码异步。

---

## 3. High

<a id="H-01"></a>
### H-01 自写剪贴板指纹去重不可靠（顺序 + 合成格式 → 粘贴回声进历史）

- **位置**：`ClipboardMonitor::WriteClipboardItem`（约 520–587）；`ClipboardFingerprint::Add` / `HashCapture`；`ReadClipboardAndSave`（约 506–512）
- **问题**：
  1. **顺序敏感**：写入侧按 `item.data`（库内 `sequence ASC`）顺序哈希；读取侧按 `EnumClipboardFormats` 顺序哈希。两者顺序不保证一致 → 即使格式集合相同也可能指纹不同。
  2. **格式集合敏感**：写入只覆盖主动 `SetClipboardData` 的格式；捕获时常含系统延迟渲染/合成格式（例如只写 `CF_UNICODETEXT` 仍枚举到 `CF_TEXT`）。
- **影响**：`was_written_by_this_process` 经常失败 → 粘贴回声被当作用户拷贝入库；与 `MarkCopied` 叠加后排序/计数异常。
- **修复方向**：
  - 使用**顺序无关**指纹（按 `(format, name)` 排序后再哈希，或按格式 XOR/多重集合哈希）；
  - 指纹只对“规范格式子集”计算，或注册私有“由 Maccy 写入”剪贴板格式；
  - 或改用 `GetClipboardSequenceNumber` + 本进程写入标记 / 短时忽略窗口。

<a id="H-02"></a>
### H-02 捕获失败会清除 `m_expectedClipboardFingerprint`

- **位置**：`ClipboardMonitor::ReadClipboardAndSave`（约 501–503）
- **问题**：`OpenClipboard` 重试失败或 `CaptureClipboard` 返回 `nullopt` 时直接 `reset()` 期望指纹。
- **影响**：刚写入剪贴板后若短暂打不开剪贴板，后续成功捕获时已无期望指纹 → 自写内容进入历史；与 H-01 叠加更明显。
- **修复方向**：仅在“确认读到了与期望不同的内容”或超时后清除；打开失败时保留期望指纹。

<a id="H-03"></a>
### H-03 `WriteClipboardItem` 中途失败导致剪贴板残缺

- **位置**：`ClipboardMonitor::WriteClipboardItem`（约 576–584）
- **问题**：`EmptyClipboard` 成功后，循环 `SetClipboardData` 若中途失败则 `return false`；已提交的格式留在剪贴板上，未提交的 `UniqueGlobal` 被析构释放；也不设置期望指纹。
- **影响**：用户剪贴板被清空或只剩部分格式；粘贴失败弹窗后系统剪贴板已损坏。
- **修复方向**：先准备全部 HGLOBAL，全部成功后再 `EmptyClipboard`+提交；失败则尽量恢复写入前快照，或至少不要在半提交状态返回。

<a id="H-04"></a>
### H-04 粘贴路径：焦点已切回目标但 `SendInput` 静默放弃

- **位置**：`PasteController::RestoreTargetFocusAndPaste`（约 101–104）
- **问题**：`SetForegroundWindow` 后若 `GetForegroundWindow` 仍不是 target（常见 UIPI / 前台锁），函数直接 return，此前可能已 `WriteClipboardItem` 改写了系统剪贴板。
- **影响**：“历史项进了剪贴板但没粘贴出去”；用户以为粘贴失败，实际剪贴板已被替换。
- **修复方向**：区分“仅复制”与“复制+粘贴”；粘贴失败时提示，或提供回滚；加强前台权限（`AllowSetForegroundWindow`、重试、`AttachThreadInput` 时机）。

<a id="H-05"></a>
### H-05 异步回调捕获 `MainWindow*` / 退出后仍可能执行

- **位置**：`MainWindow::PasteItem`、`RefreshHistory`、`LoadImageThumbnails`；`ApplicationController::SaveCapturedClipboard`
- **问题**：lambda 捕获 `this`；`Shutdown` 只把 UI HWND 置空，并不取消已入队且即将 `DrainUiCallbacks` 的回调；`storage.Stop()` 在消息循环之后。
- **影响**：退出/销毁过程中仍可能 `WriteClipboardItem`、`HideMainWindow`、`MessageBox`、改列表状态 → 半销毁对象上的逻辑错误或崩溃。
- **修复方向**：窗口 epoch / `std::weak_ptr` 守卫；回调入口检查 `m_exiting`；`DestroyWindow` 前取消并排空异步队列。

<a id="H-06"></a>
### H-06 `WH_KEYBOARD_LL` 中 `CallNextHookEx` 第一个参数传 `nullptr`

- **位置**：`KeyboardHandler::LowLevelKeyboardProc`（多处）
- **问题**：全部 `CallNextHookEx(nullptr, ...)`。文档允许，但与持有 `m_keyboardHook` 句柄的惯用法不一致；部分环境下链行为更难诊断。
- **影响**：通常可用，但与注入重放、杀软钩子共存时问题更难定位。
- **修复方向**：传入 `handler->m_keyboardHook`（注意静态回调里取当前 hook 句柄）。

<a id="H-07"></a>
### H-07 Win 键延迟重放与 `SendInput` 注入标记

- **位置**：`KeyboardHandler::LowLevelKeyboardProc`（约 203–250）
- **问题**：单独释放 Win 或 Win+其他键时，用 `SendInput` 补发 Win-down；注入事件因 `LLKHF_INJECTED` 被本钩子放行，但系统/其他钩子看到的时序与真实按键不同。
- **影响**：偶发开始菜单误开、Win+组合键失效、与游戏/其他全局钩子冲突；杀软启发式风险（项目 README 已提示）。
- **修复方向**：评估改用 `RegisterHotKey`（无 Win 修饰时）或更精确的 scan code / 扩展键标志；重放时带上原始 `scanCode`/`flags`；明确 Win down/up 配对状态机。

<a id="H-08"></a>
### H-08 单槽 `m_expectedClipboardFingerprint` 可被连续写入覆盖

- **位置**：`ClipboardMonitor.h`（约 71）；`WriteClipboardItem`（约 586）
- **问题**：只保存一个期望指纹；第一次粘贴的 `WM_CLIPBOARDUPDATE` 到达前若发生第二次写入，期望值被覆盖。
- **影响**：第一次粘贴回声可能被入库；与 H-01/H-02 叠加。
- **修复方向**：待确认自写队列 / 计数，或私有剪贴板格式携带 generation id。

<a id="H-09"></a>
### H-09 先改剪贴板、后抢焦点再 `SendInput` 的时间窗

- **位置**：`MainWindow::PasteItem`（约 1168–1193）与 `PasteController::RestoreTargetFocusAndPaste`
- **问题**：`WriteClipboardItem` 在 Maccy 仍持焦点时执行；隐藏窗口并恢复目标焦点后才注入 Ctrl+V。
- **影响**：中间窗口可被其他程序读走/覆盖剪贴板，或触发额外监听噪声。
- **修复方向**：尽量在 `SendInput` 直前写入；或写入期间开启短时 ignore + 指纹（配合 H-01）。

---

## 4. Medium

<a id="M-01"></a>
### M-01 Alt+单击托盘“暂停”不持久化

- **位置**：`MainWindow::OnTrayIcon`（约 2271–2278）对比 `OnTrayIgnoreCommand`（约 2163–2171）
- **问题**：Alt+左键切换 `ignore_events` 不调用 `PersistSettings()`；菜单项路径会持久化。
- **影响**：重启后暂停状态丢失；与剪贴板路径里“检测到 ignore 变化就 SaveSettings”行为不一致。
- **修复方向**：两条路径共用同一 toggle+persist+刷新托盘/页脚辅助函数。

<a id="M-02"></a>
### M-02 设置窗口持有 settings 副本，可被托盘状态覆盖写回

- **位置**：`SettingsWindow` 内部 `m_settings`；`MainWindow` 托盘切换 ignore
- **问题**：设置窗打开期间，托盘改了 `MainWindow::m_settings`；用户在设置窗点保存可能把旧快照写回 DB。
- **影响**：暂停/忽略等开关“改了又被设置页盖回去”。
- **修复方向**：外部变更时 `SetSettingsSnapshot`；或单一 settings 源 + 脏字段合并策略。

<a id="M-03"></a>
### M-03 `m_loadingList` 在搜索回调提前返回时可能卡住

- **位置**：`MainWindow::RefreshHistory` 回调（约 809–813）；`PasteItem` 依赖 `!m_loadingList`（约 1140）
- **问题**：generation / 查询文本 / 窗口校验失败时直接 `return`，不把 `m_loadingList` 置 false。通常后续搜索会修好；若“最新 generation 因 query 不一致被丢弃且无后续完成”，loading 一直为 true。
- **影响**：粘贴、列表选择被永久挡住，弹窗像死了一样。
- **修复方向**：loading 与 generation 绑定；仅最新 generation 负责清 loading；stale 结果忽略，最新结果被拒则清 loading 或重搜。

<a id="M-04"></a>
### M-04 UI 线程同步 `StorageWorker::Invoke` 阻塞

- **位置**：`StorageWorker.h` `Invoke`；`PersistSettings`、`TogglePin`、`DeleteItem`、`ClearHistory`、`MarkCopied`、退出清历史等
- **问题**：UI 线程 `future.get()` 等待存储线程；存储忙于大图保存/全表搜索时弹窗卡顿。
- **影响**：拖动、粘贴、钉住、删条目时 UI 冻住数百毫秒到数秒。
- **修复方向**：热路径改异步；设置保存防抖；启动仍可用同步 API。

<a id="M-05"></a>
### M-05 双 SQLite 连接（Storage + Preview）

- **位置**：`StorageWorker::ThreadMain`；`PreviewWorker::ThreadMain` 内 `Database database(m_path)`；`busy_timeout = 1000`
- **问题**：预览只读连接与存储写连接并发；忙时预览 `GetItem` 可能抛错。
- **影响**：预览间歇失败；调试输出“Preview worker stopped”。
- **修复方向**：预览走 `StorageWorker::GetItemAsync`；或只读连接 + 明确重试/降级。

<a id="M-06"></a>
### M-06 预览请求取消时不回调 UI

- **位置**：`PreviewWorker::ProcessRequest` 多处提前 `return`；`Request` 在 `PostThreadMessage` 失败时清请求（约 230–238）
- **问题**：取消/停止路径不 `PostResult`；UI 仍以为请求在飞。
- **影响**：预览候选状态陈旧；快速划过列表时状态抖动。
- **修复方向**：一律回调 `displayed=false` / `cancelled`；`Request` 返回 false 时主窗 bump generation。

<a id="M-07"></a>
### M-07 ThumbnailWorker COM 初始化失败后静默丢弃任务

- **位置**：`ThumbnailWorker::ThreadMain`（约 75–96）
- **问题**：`CoInitializeEx` 硬失败时循环不处理 job，但 `Submit` 仍接收。
- **影响**：图片缩略图永久空白，无错误提示。
- **修复方向**：失败标志；拒绝 Submit；可选一次性 UI 提示。

<a id="M-08"></a>
### M-08 跨线程创建的 `HBITMAP` 在 UI 线程绘制

- **位置**：`ThumbnailWorker` 解码；`HistoryRenderer` `StretchBlt`
- **问题**：GDI 位图跨线程使用在部分环境不稳定。
- **影响**：缩略图空白、偶发 GDI 异常。
- **修复方向**：worker 只传 DIB 字节，UI 线程 `CreateDIBSection`；或 WIC 位图在 UI 侧物化。

<a id="M-09"></a>
### M-09 `AppSettings::Save`：DB 已提交后注册表失败

- **位置**：`Settings.cpp` `AppSettings::Save`（约 249–304）
- **问题**：事务 `Commit` 后才 `SetLaunchAtLogin`；注册表失败抛异常，DB 已写入。
- **影响**：DB 与“登录启动”注册表不一致。
- **修复方向**：先注册表后 DB，或失败补偿写回。

<a id="M-10"></a>
### M-10 `SearchHistory` 每次全表装入内存再过滤

- **位置**：`Database::SearchHistory`（约 880–901 及后续）
- **问题**：`SELECT ... FROM history_items ORDER BY id DESC` 拉全表元数据，再在进程内精确/模糊/正则过滤。
- **影响**：历史上千条 + 大 preview 时存储线程占用高，UI 经 `Invoke`/异步回调间接卡顿；正则模式更差（见 M-11）。
- **修复方向**：SQL 侧过滤/排序；列表分页；模糊/正则限制扫描上限。

<a id="M-11"></a>
### M-11 用户正则 / 搜索正则可能 ReDoS

- **位置**：`ClipboardMonitor::ShouldIgnoreText`；`Database::SearchHistory` 正则分支；`HistoryRenderer` 高亮
- **问题**：`std::wregex` + `regex_search` 作用于可能很长的剪贴板文本；恶意或拙劣模式可造成灾难性回溯。
- **影响**：UI/存储线程长时间 100% CPU，应用“假死”。
- **修复方向**：超时取消、长度上限、RE2 或限制引擎、编译失败已吞掉但运行时仍要限时。

<a id="M-12"></a>
### M-12 `ignore_only_next_event` 与 ignore 状态机分散

- **位置**：`ClipboardMonitor::OnClipboardUpdate`；`ApplicationController::HandleClipboardUpdate`；设置页 `IDC_X_IGNORE_NEXT`；托盘 toggle
- **问题**：多处修改 `AppSettings` 引用字段；持久化条件不一致（有的路径 Save，有的等剪贴板事件）。
- **影响**：设置页勾选“只忽略下一次”与托盘暂停组合时难推理；重启后状态意外。
- **修复方向**：单一 `SetIgnorePolicy(...)` 负责改内存 + 可选 persist + 通知 UI。

<a id="M-13"></a>
### M-13 粘贴定时器：修饰键一直按住则最多等 3 秒后放弃粘贴

- **位置**：`PasteController::StartPasteTimer` / `OnPasteTimer`（deadline +3000ms）
- **问题**：用户按住 Ctrl/Alt/Shift 超过 3 秒，定时器停止且不再 `SendInput`。
- **影响**：快捷键打开后立刻回车粘贴、修饰键未抬起时，偶发“只复制不粘贴”。
- **修复方向**：超时仍尝试粘贴；或在隐藏弹窗时强制等 key-up；文档说明行为。

<a id="M-14"></a>
### M-14 `SendInput` 不检查返回值 / 不恢复修饰键状态

- **位置**：`PasteController.cpp` `SendControlVPaste` / `SendPasteWithHeldControl`
- **问题**：忽略 `SendInput` 返回值；未处理扩展键；未统一 key-up 残留修饰键。
- **影响**：在高完整性目标、远程桌面、部分 IME 下粘贴失败且无反馈。
- **修复方向**：检查返回值；失败上报；必要时使用 scan code。

<a id="M-15"></a>
### M-15 `GetFocus()` 在 LL 钩子中仅反映安装线程焦点

- **位置**：`KeyboardHandler::LowLevelKeyboardProc`（约 102–107）热键捕获检测
- **问题**：LL 钩子在安装线程执行，`GetFocus()` 只对 Maccy UI 线程有意义。对设置页同进程控件是对的；若未来热键控件在别的线程则会失效。
- **影响**：当前架构下多为 Low；但热键捕获依赖“可见 + 控件 ID”较脆。
- **修复方向**：显式 `m_capturingHotkey` 标志由设置窗 Set/Clear，不靠 `GetFocus` 猜测。

<a id="M-16"></a>
### M-16 弹窗打开时 `CaptureTargetWindow()` 可能已丢目标

- **位置**：`ShowMainWindow`（约 2379–2380）；热键消息带 foreground（约 159–160）
- **问题**：若热键路径未带上正确 foreground，或 shell/本窗已抢焦点，`CaptureTargetWindow` 直接 return，保留旧 target——旧 target 可能是过期 HWND。
- **影响**：粘贴到错误窗口或 `IsWindow` 失败导致不粘贴（叠加 H-04）。
- **修复方向**：热键消息始终传递捕获时的 foreground；无效 target 时明确失败提示。

<a id="M-17"></a>
### M-17 `ApplicationController::Shutdown` 不停 preview（扩大 C-01 窗口）

- **位置**：`ApplicationController.cpp`（约 60–76）
- **问题**：只 `SetUiWindow(nullptr)`，线程与预览窗仍在。
- **修复方向**：与 C-01 一并纳入统一关机顺序。

<a id="M-18"></a>
### M-18 打开热键注册失败无用户可见错误

- **位置**：`MainWindow::InitializeHistoryControls`（约 1840–1842）；`RegisterGlobalHotKey`
- **问题**：`SetWindowsHookEx` 失败时 `m_hotkeyRegistered=false`，初始化仍标记成功。
- **影响**：无全局热键，用户只能托盘打开，不易察觉原因（权限/钩子被拒）。
- **修复方向**：初始化警告托盘 balloon / 设置页状态。

<a id="M-19"></a>
### M-19 `CaptureBitmapAsDib` 拒绝负高度（自上而下 DIB）

- **位置**：`ClipboardMonitor.cpp` `CaptureBitmapAsDib`（约 86–88）
- **问题**：`source.bmHeight <= 0` 直接失败；自上而下位图在 `BITMAP` 中高度为负。
- **影响**：仅有 `CF_BITMAP`、无 DIB/DIBV5 时，部分应用图片无法入库。
- **修复方向**：用 `abs(bmHeight)` 算尺寸；`biHeight` 符号按 `GetDIBits` 要求设置。

<a id="M-20"></a>
### M-20 任一格式命中忽略列表则丢弃整次拷贝

- **位置**：`ClipboardMonitor::CaptureClipboard`（约 384–389）
- **问题**：枚举到的任一格式名匹配忽略规则即 `nullopt`，连同文本/图片一并丢弃。
- **影响**：忽略厂商元数据格式时，用户可见正文也进不了历史。
- **修复方向**：产品确认是否有意；否则改为按格式跳过存储，而非整次放弃。

<a id="M-21"></a>
### M-21 `ignore_only_next` 有 DDX/资源 ID，但 `.rc` 无对应控件

- **位置**：`SettingsWindow.h` DDX `IDC_X_IGNORE_NEXT`；`resource.h`；`SettingsWindow.rc` 高级页仅有 `IDC_X_IGNORE_EVENTS`
- **问题**：代码与持久化键存在，设置界面无勾选框；用户无法在 UI 配置“只忽略下一次”。
- **影响**：半死功能；仅能靠 DB/默认值生效，行为难理解（与 M-12、M-22 叠加）。
- **修复方向**：补全 RC 控件，或删除设置项与 DDX。

<a id="M-22"></a>
### M-22 `ignore_only_next_event` 仅在 `ignore_events==true` 时生效

- **位置**：`ClipboardMonitor::OnClipboardUpdate`（约 600–606）
- **问题**：外层先判断 `ignore_events`；“只忽略下一次”不能独立跳过单次拷贝。
- **影响**：若产品语义是“跳过下一次拷贝”，当前实现不满足（除非同时开启暂停）。
- **修复方向**：独立处理 one-shot；或明确文档为“暂停的自动解除开关”并修好 UI（M-21）。

<a id="M-23"></a>
### M-23 `m_pasteInProgress` 在粘贴完成前过早清除

- **位置**：`MainWindow::PasteItem` 异步回调（约 1163 vs 1193）
- **问题**：回调入口即 `m_pasteInProgress = false`，随后才 `WriteClipboardItem` / `RestoreTargetFocusAndPaste`（含 3s 定时器路径）。
- **影响**：用户可触发第二次粘贴，覆盖剪贴板并搅乱期望指纹（H-08）。
- **修复方向**：标志保持到粘贴/定时器结束或明确失败。

<a id="M-24"></a>
### M-24 `PasteModifiersDown` 把 Shift 当作阻塞修饰键

- **位置**：`PasteController.cpp`（约 116–118、105–110）
- **问题**：仍按住 Shift（选区快捷键残留）时走 3s 等待定时器，而非立即 Ctrl+V。
- **影响**：粘贴变慢或超时放弃（与 M-13 叠加）。
- **修复方向**：仅对会破坏 Ctrl+V 的修饰键（如 Alt）等待；或只在“Control 已按下”路径用短注入。

<a id="M-25"></a>
### M-25 `save_text` 关闭时正则忽略不检查 HTML/RTF 正文

- **位置**：`ClipboardMonitor::CaptureClipboard`（约 392–403）
- **问题**：`save_text==false` 时仍为正则提取纯文本；无 Unicode/ANSI 文本则不对富文本做 `ShouldIgnoreText`。
- **影响**：敏感内容仅以 HTML/RTF 存在时绕过忽略规则。
- **修复方向**：忽略检查使用 `SearchPartsFromFormats`（与是否入库解耦）。

<a id="M-26"></a>
### M-26 默认忽略格式名 `CF_CLIPBOARD_VIEWER_IGNORE` 可能匹配不上

- **位置**：`ClipboardRules::DefaultIgnoredFormats`；`FormatName` / `ShouldIgnoreFormat`
- **问题**：注册格式经 `GetClipboardFormatNameW` 返回的名称常非字面 `CF_*` 别名。
- **影响**：内置“忽略 viewer-ignore 标记”可能从不生效。
- **修复方向**：按已注册 format ID 比较，或维护已知别名表。

---

## 5. Low

<a id="L-01"></a>
### L-01 `PreviewWorker::Reposition` 使用线程启动时缓存的 owner

- **位置**：`PreviewWorker::ThreadMain` `kRepositionCommand`（约 324–327）对比 `ProcessRequest` 使用 `m_ownerWindow.load()`
- **问题**：reposition 用局部 `owner`，非原子加载的当前 owner。
- **影响**：当前不会换 owner；一旦换就会用到死句柄。
- **修复方向**：统一 `m_ownerWindow.load()`。

<a id="L-02"></a>
### L-02 `StorageWorker::Stop` 丢弃未执行的 UI 回调

- **位置**：`StorageWorker::Stop`（约 56–58）
- **问题**：清空 `m_uiCallbacks` 不执行。
- **影响**：退出时可接受；顺序错误时与 H-05 叠加。
- **修复方向**：明确关机协议：先禁投递 → 排空安全回调 → join。

<a id="L-03"></a>
### L-03 调整大小命中边框未按 DPI 缩放

- **位置**：`MainWindow.cpp` `kResizeBorder = 8`；`OnNcHitTest`
- **影响**：高 DPI 下难拖拽边缘。
- **修复方向**：`MulDiv` / `UiFont::DpiForWindow` 缩放。

<a id="L-04"></a>
### L-04 托盘图标 mask 位图可能透明通道不正确

- **位置**：`TrayIcon.cpp` `CreateIconFromPngResource`（约 189–205）
- **问题**：`CreateBitmap` 1-bit mask 未按 alpha 生成。
- **影响**：部分主题下托盘图标发灰/毛边。
- **修复方向**：由预乘 alpha 生成正确 AND/XOR mask，或使用支持 alpha 的图标创建路径。

<a id="L-05"></a>
### L-05 `UpdateChecker::Start` 在 UI 线程 `join` 上一次检查

- **位置**：`UpdateChecker.cpp`（约 252–255）
- **影响**：连续点“检查更新”可能卡住至网络超时。
- **修复方向**：检查中禁用按钮；join 放到非 UI 或仅 Stop 时 join。

<a id="L-06"></a>
### L-06 `SaveClipboard` 指纹命中且已有 formats 时不更新 payload/FTS

- **位置**：`Database::SaveClipboard`（约 806–809）
- **问题**：同指纹跳过 `ReplaceFormats` / `ReplaceSearchIndex`。
- **影响**：指纹算法变更或碰撞时留下旧 blob；一般可接受。
- **修复方向**：确保指纹覆盖完整 payload；必要时比较大小/校验和。

<a id="L-07"></a>
### L-07 `main.cpp` 异常路径依赖析构顺序停线程

- **位置**：`wWinMain` catch（约 368–376）
- **问题**：未显式 `storage.Stop()` / `preview.Stop()`。
- **影响**：当前栈上对象析构顺序通常正确；重构易踩坑。
- **修复方向**：scope_guard / 显式 stop。

<a id="L-08"></a>
### L-08 指纹算法为 64-bit FNV 变体，理论碰撞

- **位置**：`ClipboardMonitor.cpp` `ClipboardFingerprint`
- **影响**：不同内容映射同指纹会错误合并历史（更新同一行）。概率低但大数据量下非零。
- **修复方向**：加密哈希（BLAKE3/SHA-256 截断）或增加长度/格式计数混入。

<a id="L-09"></a>
### L-09 `CF_BITMAP` → DIB 转换丢弃调色板/特殊格式位图

- **位置**：`CaptureBitmapAsDib`（强制 32bpp BI_RGB）
- **影响**：极端位图捕获失败被跳过；有 DIB/DIBV5 时已优先；与 M-19（负高度）叠加时更容易丢图。
- **修复方向**：优先使用已有 DIB；查询真实 bit count/调色板后再 `GetDIBits`；转换失败记日志。

<a id="L-10"></a>
### L-10 `FilesPreview` / 枚举文件路径无总长度上限

- **位置**：`ClipboardMonitor::FilesPreview`
- **问题**：大量文件路径拼进 preview 字符串。
- **影响**：内存与 DB preview 膨胀（另有 StoredPreview 截断缓解）。
- **修复方向**：与 `ClipboardRules::Limits` 对齐截断。

<a id="L-11"></a>
### L-11 忽略应用规则在“白名单模式”下空 application 一律忽略

- **位置**：`ShouldIgnoreApplication`（约 206–208）
- **问题**：拿不到源进程路径时，白名单模式丢弃该次拷贝。
- **影响**：部分 UWP/服务/延迟渲染场景历史缺失（可能是有意策略）。
- **修复方向**：产品确认；或回退到窗口类名规则。

<a id="L-12"></a>
### L-12 `RegisterGlobalHotKey` 名实不符

- **位置**：`KeyboardHandler::RegisterGlobalHotKey`
- **问题**：实际是 `SetWindowsHookEx(WH_KEYBOARD_LL)`，不是 `RegisterHotKey`。
- **影响**：可读性/维护误导；与 `WM_HOTKEY` 处理并存易混淆。
- **修复方向**：重命名为 `InstallOpenHotkeyHook`；评估非 Win 修饰键改用 `RegisterHotKey`。

<a id="L-13"></a>
### L-13 托盘未设置 `NOTIFYICON_VERSION_4`

- **位置**：`TrayIcon::Add`
- **影响**：部分 Win10/11 行为（如现代化通知）不一致；功能基本可用。
- **修复方向**：`NIM_SETVERSION` + `NOTIFYICON_VERSION_4`。

<a id="L-14"></a>
### L-14 `HasPendingKeyboardInput` 使用 `PeekMessage` 看键盘队列

- **位置**：`MainWindow.cpp`（约 89–94）
- **问题**：仅窥视该 HWND 队列；IME/别的输入路径可能漏判。
- **影响**：搜索结果提交时机略抖。
- **修复方向**：结合 IME composition 标志（项目已有部分 IME 处理）。

<a id="L-15"></a>
### L-15 消息 ID 间隙与注释

- **位置**：`Constants.h`：`kInstallerShutdownMessage = WM_APP+8` 夹在 +6 与 +9 之间；`kThumbnailWorkerResultMessage = WM_APP+11`
- **影响**：无功能问题；扩展时易撞号。
- **修复方向**：整理连续分配表。

<a id="L-16"></a>
### L-16 `PinKeys` 可用键集合固定且排除 a/q/v/w/z 等

- **位置**：`PinKeys.cpp` `kSupportedKeys`
- **影响**：钉住项较多时 `Next` 返回空（已有 MessageBox）；与快捷键字母冲突策略需用户理解。
- **修复方向**：设置页展示可用键；支持自定义 pin 键表。

<a id="L-17"></a>
### L-17 HTML/RTF 剥离启发式粗糙

- **位置**：`ClipboardRules::StripHtml` / `StripRtf`
- **影响**：搜索正文质量一般；不造成崩溃。
- **修复方向**：按需增强；保持长度上限。

<a id="L-18"></a>
### L-18 WTL 规范符合度（工程向）

- **对照**：`WTL.md`
- **观察**：主窗已用 `CDialogImpl` + 消息映射；仍有大量直接 Win32（钩子、剪贴板、托盘）——按规范属于合理层。设置/历史以 `.rc` 驱动，方向正确。
- **问题**：`KeyboardHandler` 回调函数指针 C 风格 API 偏“重包装”；与项目“少层”原则略冲突，但非缺陷。
- **修复方向**：长期可改为 `std::function` 或直接友元/`MainWindow` 方法，减少 void* 回调表。

<a id="L-19"></a>
### L-19 测试覆盖缺口

- **现有**：`CoreRulesTests`、`FileHistoryTests`、`UpdateCheckerTests`
- **缺口**：无针对 `WriteClipboardItem` 回声去重、`PasteController` 焦点失败、`m_loadingList`、关机顺序的自动化测试。
- **修复方向**：为指纹/合成格式、ignore 状态机加单元测试；关键路径加手工测试清单。

<a id="L-20"></a>
### L-20 页脚焦点下 Enter 无动作

- **位置**：`KeyboardHandler::HandlePopupKey`（约 510–512）
- **问题**：`m_activeFooter >= 0` 时对 `VK_RETURN` 分支为空。
- **影响**：键盘无法激活页脚按钮（清除/设置等）。
- **修复方向**：调用对应 footer 命令回调。

<a id="L-21"></a>
### L-21 数字小键盘无法触发序号快捷粘贴

- **位置**：`KeyboardHandler::HandlePopupShortcut`（约 594–600）
- **问题**：仅匹配 `'0'+n`，未映射 `VK_NUMPAD0`–`9`。
- **影响**：小键盘用户无法按数字快粘。
- **修复方向**：归一化数字键 / 增加 NUMPAD 映射。

<a id="L-22"></a>
### L-22 `ClipboardGuard` 仅 `OpenClipboard(owner)`，无 `NULL` 回退

- **位置**：`ClipboardMonitor.cpp`（约 29–31）
- **问题**：关机或 owner 无效时打开失败率略高。
- **影响**：边缘打开失败，放大 H-02。
- **修复方向**：必要时回退 `OpenClipboard(nullptr)`；`Shutdown` 后勿再打开。

---

## 6. 按子系统汇总

### 6.1 剪贴板监控与规则（`ClipboardMonitor` / `ClipboardRules`）

| ID | 级别 | 摘要 |
|----|------|------|
| H-01 | High | 自写指纹：顺序 + 合成格式 |
| H-02 | High | 捕获失败清除期望指纹 |
| H-03 | High | 半提交写入 |
| H-08 | High | 单槽期望指纹被覆盖 |
| M-11 | Medium | 忽略正则 ReDoS |
| M-12 / M-21 / M-22 | Medium | ignore / ignore-next 状态与 UI |
| M-19 / M-20 / M-25 / M-26 | Medium | 位图负高、整夹忽略、富文本正则、默认格式名 |
| L-08–L-11, L-17, L-22 | Low | 哈希/位图/路径/白名单/HTML/OpenClipboard |

### 6.2 粘贴（`PasteController` + `MainWindow::PasteItem`）

| ID | 级别 | 摘要 |
|----|------|------|
| H-04 | High | 前台失败静默、剪贴板已改 |
| H-05 | High | 异步粘贴回调生命周期 |
| H-09 | High | 先写剪贴板后抢焦点时间窗 |
| M-13 / M-24 | Medium | 修饰键等待超时 / Shift 误阻塞 |
| M-14 | Medium | SendInput 无校验 |
| M-16 | Medium | 粘贴目标 HWND 过期 |
| M-23 | Medium | pasteInProgress 过早清除 |

### 6.3 键盘钩子（`KeyboardHandler`）

| ID | 级别 | 摘要 |
|----|------|------|
| H-06 | High | CallNextHookEx(nullptr) |
| H-07 | High | Win 键重放时序 |
| M-15 | Medium | GetFocus 热键捕获 |
| M-18 | Medium | 钩子安装失败无提示 |
| L-12 / L-20 / L-21 | Low | API 命名；页脚 Enter；小键盘序号粘贴 |

### 6.4 主窗 / 历史 UI（`MainWindow` / `HistoryListControls` / `HistoryRenderer`）

| ID | 级别 | 摘要 |
|----|------|------|
| M-01 | Medium | 托盘暂停不持久化 |
| M-02 | Medium | 设置副本覆盖 |
| M-03 | Medium | loadingList 卡住挡粘贴 |
| M-04 | Medium | UI 阻塞 Invoke |
| L-03, L-14 | Low | DPI 边框 / PeekMessage |

### 6.5 存储与搜索（`Database` / `StorageWorker`）

| ID | 级别 | 摘要 |
|----|------|------|
| M-05 | Medium | 双连接 SQLITE_BUSY |
| M-10 | Medium | 全表搜索 |
| M-11 | Medium | 搜索正则 ReDoS |
| L-02, L-06 | Low | Stop 丢回调 / 不更新 formats |

### 6.6 预览与缩略图（`PreviewWorker` / `ThumbnailWorker`）

| ID | 级别 | 摘要 |
|----|------|------|
| C-01 | Critical | 销毁顺序 |
| C-02 | Critical | 跨线程 UI |
| M-06 | Medium | 取消无回调 |
| M-07 | Medium | COM 失败静默 |
| M-08 | Medium | 跨线程 HBITMAP |
| M-17 | Medium | Shutdown 不停 preview |
| L-01 | Low | reposition stale owner |

### 6.7 设置 / 托盘 / 更新 / 启动

| ID | 级别 | 摘要 |
|----|------|------|
| M-09 | Medium | 注册表与 DB 不一致 |
| L-04 | Low | 托盘 mask |
| L-05 | Low | UpdateChecker join 阻塞 |
| L-07 | Low | main 异常路径 |
| L-13 | Low | NIM 版本 |
| L-15–L-16, L-18–L-19 | Low | 常量/钉住键/WTL/测试 |

---

## 7. 建议修复优先级（实施顺序）

1. **关机与预览线程**：C-01、C-02、M-17、H-05（先消除退出崩溃面）
2. **剪贴板回声**：H-01（顺序无关指纹）、H-02、H-03、H-08（消除粘贴污染历史）
3. **粘贴可靠性**：H-04、H-09、M-13、M-14、M-16、M-23、M-24
4. **UI 状态一致性**：M-01、M-02、M-03、M-12、M-21、M-22
5. **捕获正确性**：M-19、M-20、M-25、M-26
6. **性能与卡顿**：M-04、M-10、M-11
7. **其余 Medium/Low**：按产品痛点插入（含 L-20 页脚 Enter、L-21 小键盘）

---

## 8. 结论

代码在单实例、存储线程串行化 SQLite、搜索 generation、历史列表 owner-draw、忽略规则等方面有明显工程意识，不是粗糙原型。

当前最需要优先处理的是：**预览窗跨线程生命周期**、**自写剪贴板回声去重（含枚举顺序）**、**粘贴失败时已改剪贴板**、**异步回调与退出竞态**。这四类会直接体现为崩溃、历史重复、粘贴“失灵”。

本报告为静态审计结果；对 H-01 / C-01 / H-04 建议在修复前用最小复现用例做一次运行时确认（粘贴文本后是否新增历史行、退出时是否偶发挂起、前台锁定应用中粘贴是否只改剪贴板）。

---

## 9. 修订记录

| 日期 | 说明 |
|------|------|
| 2026-11-03 | 初版：主路径 + UI/存储/预览综合审计 |
| 2026-11-03 | 合并剪贴板专项补充：H-01 明确顺序无关性；新增 H-08/H-09、M-19–M-26、L-20–L-22；核实 `SettingsWindow.rc` 确无 `IDC_X_IGNORE_NEXT` 控件 |
| 2026-11-03 | 为全部发现项加入 HTML 锚点；修复进度改由 `FIX_CHECKLIST.md` 跟踪 |

*生成说明：基于仓库源码静态分析整理（含专项子审计交叉核对）；若后续代码变更，请以 git 历史对照本报告条目是否仍适用。*
