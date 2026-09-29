## WTL 开发规范

本项目是 Windows 原生桌面应用，UI 基于 **WTL（Windows Template Library）**。

### 核心原则

- **优先使用 WTL 简化 Win32 开发。**
- 能用 WTL 完成的功能，不要直接编写冗长的 Win32 API 封装。
- 优先使用 WTL 提供的窗口类、消息映射、控件封装、对话框、布局和 GDI 封装。
- 保持原生 Win32/WTL 架构，不要为了减少代码引入重量级 UI 框架。
- 不要为了“现代化”而引入 Qt、MFC、wxWidgets、Electron、WebView、ImGui 等框架。
- WTL 不支持或无法合理简化的功能，才直接使用 Win32 API。
- 如果直接调用 Win32 API 可以解决底层问题，可以使用，但应避免在业务代码中重复封装已有的 WTL 能力。

### 技术选择优先级

实现 Windows UI 或窗口逻辑时，按照以下顺序选择：

1. WTL 现有类和工具
2. Windows Common Controls
3. Win32 API
4. 自定义 Win32 封装

例如：

```text
窗口            → CWindowImpl / CDialogImpl
对话框          → CDialogImpl
消息处理        → BEGIN_MSG_MAP / MESSAGE_HANDLER / COMMAND_HANDLER
按钮/编辑框等   → WTL 控件类
窗口居中        → WTL / Win32 简单 API
字体            → CFont
画刷            → CBrush
画笔            → CPen
设备上下文      → CDC / CClientDC
字符串          → CString
窗口句柄        → HWND / CWindow
定时器          → WTL 消息映射 + Win32 SetTimer
托盘图标        → WTL 提供的托盘相关能力；若不足再使用 Shell_NotifyIcon
```

### 窗口开发

优先使用 WTL 窗口类：

```cpp
class CMainWindow :
    public CWindowImpl<CMainWindow>
{
public:
    DECLARE_WND_CLASS(L"MyApp.MainWindow")

    BEGIN_MSG_MAP(CMainWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        COMMAND_HANDLER(IDC_BUTTON, BN_CLICKED, OnButtonClicked)
    END_MSG_MAP()

private:
    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnButtonClicked(WORD, WORD, HWND, BOOL&);
};
```

不要把窗口过程写成手工 `WndProc`，除非存在明确的技术原因。

不推荐：

```cpp
LRESULT CALLBACK WndProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message) {
        ...
    }
}
```

如果 `CWindowImpl` 可以解决问题，不要退回手写 `WndProc`。

### 消息处理

优先使用 WTL 消息映射。

推荐：

```cpp
BEGIN_MSG_MAP(CMainWindow)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_CLOSE, OnClose)
    COMMAND_HANDLER(IDC_OK, BN_CLICKED, OnOk)
END_MSG_MAP()
```

避免在一个大型 `switch (message)` 中处理所有窗口消息。

消息处理函数保持短小：

```cpp
LRESULT OnOk(WORD, WORD, HWND, BOOL&)
{
    SaveSettings();
    CloseDialog(IDOK);
    return 0;
}
```

复杂业务逻辑放到独立函数或业务类中，不要堆在消息处理函数里。

### 对话框

优先使用：

```cpp
CDialogImpl<T>
```

以及：

```cpp
DoModal()
Create()
DestroyWindow()
EndDialog()
```

不要为了一个简单设置窗口自己创建完整的窗口类、注册窗口类和消息循环。

例如：

```cpp
class CSettingsDialog :
    public CDialogImpl<CSettingsDialog>
{
public:
    enum { IDD = IDD_SETTINGS };

    BEGIN_MSG_MAP(CSettingsDialog)
        COMMAND_ID_HANDLER(IDOK, OnOk)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP();

    LRESULT OnOk(WORD, WORD, HWND, BOOL&)
    {
        SaveSettings();
        EndDialog(IDOK);
        return 0;
    }

    LRESULT OnCancel(WORD, WORD, HWND, BOOL&)
    {
        EndDialog(IDCANCEL);
        return 0;
    }
};
```

### 控件操作

优先使用 WTL 控件封装，而不是到处直接调用：

```cpp
GetDlgItem()
SendMessage()
```

例如：

```cpp
CEdit edit = GetDlgItem(IDC_EDIT_NAME);
edit.SetWindowText(L"Hello");
```

而不是：

```cpp
HWND hwnd = GetDlgItem(m_hWnd, IDC_EDIT_NAME);
SetWindowText(hwnd, L"Hello");
```

对于列表、树、Tab、ListView 等 Common Controls，优先使用 WTL 对应封装：

```cpp
CListViewCtrl
CTreeViewCtrl
CTabCtrl
CComboBox
CEdit
CButton
CStatic
```

### 字符串

Windows UI 字符串优先使用：

```cpp
CString
```

避免在 UI 层混用：

```cpp
std::wstring
std::string
LPCWSTR
LPWSTR
```

如果业务层已经使用 `std::wstring`，不需要为了 WTL 强行转换整个项目。

WTL 与 STL 可以共存。

原则是：

```text
UI 层       → CString / WTL
业务层      → STL
Windows API → HWND / LPCWSTR 等
```

不要为了“统一”而增加没有价值的字符串转换。

### GDI

优先使用 WTL 的 GDI 封装：

```cpp
CDC
CClientDC
CPaintDC
CBrush
CPen
CFont
CBitmap
```

例如：

```cpp
CPaintDC dc(m_hWnd);

CFont font;
font.CreatePointFont(90, L"Segoe UI");

dc.SelectFont(font);
dc.DrawText(
    L"Hello",
    -1,
    &rect,
    DT_CENTER | DT_VCENTER | DT_SINGLELINE
);
```

不要为简单绘制创建一层自己的 GDI Wrapper。

### 布局

优先使用 WTL 的布局能力。

对于固定尺寸、简单窗口：

```text
资源编辑器 / 对话框模板
```

优先于运行时手工创建全部控件。

对于需要动态布局的窗口：

```text
WTL Layout / AtlCtrlW
```

优先于大量：

```cpp
SetWindowPos(...)
MoveWindow(...)
```

如果必须使用 `SetWindowPos`，集中处理布局，不要让每个控件的事件函数都修改其他控件的位置。

### 资源文件

UI 优先使用 `.rc` 资源：

```text
IDD_SETTINGS
IDD_HISTORY
IDC_EDIT_SEARCH
IDC_LIST_HISTORY
IDC_BUTTON_CLEAR
```

不要在 C++ 中手工创建大量静态 UI 控件。

优先：

```text
资源模板
    ↓
WTL Dialog
    ↓
WTL 控件封装
```

而不是：

```text
C++ 创建窗口
    ↓
CreateWindowEx
    ↓
设置字体
    ↓
设置位置
    ↓
设置样式
    ↓
处理消息
```

### Win32 API 使用原则

Win32 API 并不是禁止使用。

当 WTL 没有对应能力，或者直接使用 Win32 API 更简单时，可以直接使用。

例如：

```cpp
RegisterHotKey()
UnregisterHotKey()
Shell_NotifyIcon()
GetWindowLongPtr()
SetWindowLongPtr()
SetTimer()
KillTimer()
OpenProcess()
CreateProcess()
```

但不要因为 Win32 API 存在，就绕过 WTL。

判断标准：

```text
WTL 可以直接解决？
    ↓
是 → 使用 WTL
    ↓
否
    ↓
Win32 API 可以简单解决？
    ↓
是 → 使用 Win32 API
    ↓
否
    ↓
再考虑自行封装
```

### 避免重复封装

在添加 Helper、Wrapper、Manager 之前，先检查 WTL 是否已经提供对应能力。

例如不要轻易创建：

```cpp
WindowHelper
DialogHelper
ControlHelper
StringHelper
GdiHelper
MessageHelper
```

如果只是把 WTL / Win32 API 再包一层，并没有减少复杂度，则不要创建。

优先直接使用：

```cpp
CWindow
CDialogImpl
CWindowImpl
CString
CDC
CFont
CBrush
CListViewCtrl
```

### 代码规模原则

优先选择**代码最少、依赖最少、生命周期最清晰**的实现。

如果两个方案功能相同：

```text
方案 A：100 行 Win32 样板代码
方案 B：30 行 WTL
```

选择 WTL。

如果：

```text
方案 A：20 行 WTL + 复杂模板代码
方案 B：10 行直接 Win32 API
```

选择更简单的方案。

目标不是“所有代码都必须 WTL”，而是：

> **优先利用 WTL 消除 Win32 样板代码，同时保持原生 Windows 架构。**

### 修改现有代码

修改 UI 时：

1. 先确认当前窗口使用的 WTL 基类。
2. 检查现有消息映射。
3. 优先复用现有 WTL 控件。
4. 优先修改 `.rc` 资源，而不是在 C++ 中重新创建 UI。
5. 不要因为一个小功能重写整个窗口。
6. 不要引入新的 UI 框架。
7. 不要把简单 WTL 代码替换成更复杂的自定义封装。

### Agent 行为要求

在实现 Windows UI 功能前，Agent 应先判断：

```text
这个问题 WTL 是否已经解决？
```

如果答案是“是”，直接使用 WTL。

如果答案是“不确定”，先检查项目现有代码和 WTL API，再决定是否调用底层 Win32 API。

不要默认选择：

```cpp
CreateWindowEx
SetWindowLongPtr
SendMessage
GetDlgItem
MoveWindow
WndProc
```

这些 API 应作为底层能力使用，而不是 UI 层的默认开发方式。

### 最终目标

本项目追求：

```text
原生 Win32 性能
        +
WTL 的轻量封装
        +
资源驱动 UI
        +
少量 C++ 代码
        +
清晰的消息映射
        +
低依赖
```

**能用 WTL 一行解决的问题，不要写十行 Win32。**