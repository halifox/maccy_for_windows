#define NOMINMAX
#include "SettingsWindow.h"

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace {

enum SettingsControlId : int {
    kTabs = IDC_SETTINGS_TABS,

    kGLaunch = IDC_G_LAUNCH,
    kGUpdates,
    kGCheckNow,
    kGOpenHotKey,
    kGPinHotKey,
    kGDeleteHotKey,
    kGPreviewHotKey,
    kGSearchMode,
    kGPasteByDefault,
    kGRemoveFormatting,
    kGNotifications,

    kAPopupPosition = IDC_A_POPUP_POSITION,
    kAPopupScreen,
    kAResetPosition,
    kAPinTo,
    kAImageHeight,
    kAOpenPreview,
    kAPreviewDelay,
    kAHighlight,
    kAMenuIcon,
    kAShowStatus,
    kAShowRecent,
    kAShowSearch,
    kASearchVisibility,
    kAShowSpecial,
    kAShowTitle,
    kAShowIcons,
    kAShowSwatch,
    kAShowFooter,

    kSSaveFiles = IDC_S_SAVE_FILES,
    kSSaveImages,
    kSSaveText,
    kSHistorySize,
    kSSortBy,

    kIgnoreTabs = IDC_IGNORE_TABS,
    kIList,
    kIEdit,
    kIAdd,
    kIBrowse,
    kIUpdate,
    kIRemove,
    kIReset,
    kIWhitelist,

    kPList = IDC_P_LIST,
    kPKey,
    kPTitle,
    kPContent,
    kPSave,
    kPDelete,

    kXIgnoreEvents = IDC_X_IGNORE_EVENTS,
    kXIgnoreNext,
    kXClearOnQuit,
    kXClearClipboard,
};

constexpr int kPageGeneral = 0;
constexpr int kPageAppearance = 1;
constexpr int kPageStorage = 2;
constexpr int kPageIgnore = 3;
constexpr int kPagePins = 4;
constexpr int kPageAdvanced = 5;
// The page coordinate system is 760 logical pixels wide.  Reserve space for
// the dialog frame and the tab control inset so the right-most controls still
// fit at the minimum window size.
constexpr int kMinimumSettingsWidth = 840;
constexpr int kMinimumSettingsHeight = 640;

// A Win32 drop-down combo box uses its creation/layout height for the full
// expanded control, including the list that is normally hidden.  Keep enough
// room for several rows so the list does not collapse to zero height when the
// visible selection field is only about 30 pixels tall.
constexpr int kComboTotalHeight = 180;

UINT WindowDpi(HWND window) {
    if (window != nullptr) {
        HDC dc = ::GetDC(window);
        if (dc != nullptr) {
            const int dpi = ::GetDeviceCaps(dc, LOGPIXELSX);
            ::ReleaseDC(window, dc);
            if (dpi > 0) {
                return static_cast<UINT>(dpi);
            }
        }
    }
    return USER_DEFAULT_SCREEN_DPI;
}

std::wstring ReadWindowText(HWND window) {
    if (window == nullptr) {
        return {};
    }
    const int length = GetWindowTextLengthW(window);
    if (length <= 0) {
        return {};
    }
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = GetWindowTextW(window, text.data(), length + 1);
    text.resize(static_cast<size_t>(std::max(copied, 0)));
    return text;
}

int ReadValidatedInteger(HWND window, int fallback, int minimum, int maximum) {
    const int safe_fallback = std::clamp(fallback, minimum, maximum);
    const std::wstring text = ReadWindowText(window);
    try {
        size_t parsed = 0;
        const long long raw = std::stoll(text, &parsed);
        if (parsed != text.size()) {
            throw std::invalid_argument("invalid integer");
        }
        const int value = static_cast<int>(std::clamp(
            raw,
            static_cast<long long>(minimum),
            static_cast<long long>(maximum)
        ));
        ::SetWindowTextW(window, std::to_wstring(value).c_str());
        return value;
    } catch (...) {
        ::SetWindowTextW(window, std::to_wstring(safe_fallback).c_str());
        return safe_fallback;
    }
}

void SetCheck(HWND window, bool checked) {
    if (window != nullptr) {
        SendMessageW(window, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

bool IsChecked(HWND window) {
    return window != nullptr && SendMessageW(window, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

int ComboSelection(HWND window, int fallback = 0) {
    const LRESULT result = window == nullptr ? CB_ERR : SendMessageW(window, CB_GETCURSEL, 0, 0);
    return result == CB_ERR ? fallback : static_cast<int>(result);
}

void AddComboItem(HWND combo, const wchar_t *text) {
    SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text));
}

void SelectCombo(HWND combo, int index) {
    if (combo != nullptr) {
        SendMessageW(combo, CB_SETCURSEL, index, 0);
    }
}

void SetControlFont(HWND window) {
    if (window != nullptr) {
        SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    }
}

std::wstring PinDisplay(const ClipboardItem &item) {
    std::wstring value = item.pin + L"  |  " + item.title;
    if (!item.content.empty()) {
        value += L"  |  " + item.content.substr(0, 100);
    }
    return value;
}

std::wstring PinTextContent(const ClipboardItem &item) {
    for (const ClipboardFormatData &data : item.data) {
        if (data.format == CF_UNICODETEXT || data.name == L"CF_UNICODETEXT") {
            if (data.bytes.size() < sizeof(wchar_t)) {
                return {};
            }
            const auto *text = reinterpret_cast<const wchar_t *>(data.bytes.data());
            size_t length = 0;
            const size_t capacity = data.bytes.size() / sizeof(wchar_t);
            while (length < capacity && text[length] != L'\0') {
                ++length;
            }
            return std::wstring(text, length);
        }
    }
    return {};
}

} // namespace

void SettingsPageWindow::Configure(HWND owner, bool scrollable, int design_width, int design_height) {
    m_owner = owner;
    m_scrollable = scrollable;
    m_designWidth = std::max(1, design_width);
    m_designHeight = std::max(1, design_height);
    m_scrollY = 0;
    m_dpi = USER_DEFAULT_SCREEN_DPI;
}

int SettingsPageWindow::Scale(int value) const noexcept {
    return MulDiv(value, static_cast<int>(m_dpi), USER_DEFAULT_SCREEN_DPI);
}

void SettingsPageWindow::UpdateDpi() {
    if (m_hWnd == nullptr) {
        return;
    }
    HDC dc = ::GetDC(m_hWnd);
    if (dc != nullptr) {
        const int dpi = ::GetDeviceCaps(dc, LOGPIXELSX);
        ::ReleaseDC(m_hWnd, dc);
        if (dpi > 0) {
            m_dpi = static_cast<UINT>(dpi);
        }
    }
}

void SettingsPageWindow::AddLayout(
    HWND window,
    RECT design,
    bool stretch_width,
    bool stretch_height,
    bool combo
) {
    if (window == nullptr) {
        return;
    }
    m_controls.push_back(LayoutControl{window, design, stretch_width, stretch_height, combo});
}

void SettingsPageWindow::SetContentSize(int design_width, int design_height) {
    m_designWidth = std::max(1, design_width);
    m_designHeight = std::max(1, design_height);
    LayoutControls();
}

int SettingsPageWindow::MaxScrollPosition(const RECT &client) const {
    if (!m_scrollable) {
        return 0;
    }
    return std::max(0, Scale(m_designHeight) - static_cast<int>(client.bottom));
}

void SettingsPageWindow::SetScrollPosition(int position) {
    if (m_hWnd == nullptr) {
        return;
    }
    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    const int next = std::clamp(position, 0, MaxScrollPosition(client));
    if (next != m_scrollY) {
        m_scrollY = next;
        LayoutControls();
    }
}

void SettingsPageWindow::ScrollBy(int delta) {
    SetScrollPosition(m_scrollY + delta);
}

void SettingsPageWindow::LayoutControls() {
    if (m_hWnd == nullptr) {
        return;
    }

    UpdateDpi();
    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    const int content_height = std::max(
        static_cast<int>(client.bottom),
        Scale(m_designHeight)
    );
    m_scrollY = std::clamp(m_scrollY, 0, MaxScrollPosition(client));

    SCROLLINFO scroll_info{sizeof(scroll_info)};
    scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll_info.nMin = 0;
    scroll_info.nMax = std::max(0, content_height - 1);
    scroll_info.nPage = static_cast<UINT>(std::max<LONG>(0, client.bottom));
    scroll_info.nPos = m_scrollY;
    ::SetScrollInfo(m_hWnd, SB_VERT, &scroll_info, TRUE);
    ::ShowScrollBar(m_hWnd, SB_VERT, m_scrollable && content_height > client.bottom);

    for (const LayoutControl &layout : m_controls) {
        RECT target{};
        target.left = Scale(layout.design.left);
        target.top = Scale(layout.design.top) - m_scrollY;
        target.right = layout.stretch_width
            ? client.right - Scale(layout.design.right)
            : Scale(layout.design.right);
        target.bottom = layout.stretch_height
            ? content_height - Scale(layout.design.bottom) - m_scrollY
            : Scale(layout.design.bottom) - m_scrollY;
        if (layout.combo && !layout.stretch_height) {
            target.bottom = target.top + std::max(
                Scale(kComboTotalHeight),
                static_cast<int>(target.bottom - target.top)
            );
        }

        ::SetWindowPos(
            layout.window,
            nullptr,
            target.left,
            target.top,
            std::max<int>(0, target.right - target.left),
            std::max<int>(0, target.bottom - target.top),
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }
}

LRESULT SettingsPageWindow::OnSize(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    LayoutControls();
    return 0;
}

LRESULT SettingsPageWindow::OnVScroll(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    handled = TRUE;
    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    const int line = std::max(1, Scale(32));
    const int page = std::max(1, static_cast<int>(client.bottom) - line);
    switch (LOWORD(wParam)) {
    case SB_LINEUP:
        ScrollBy(-line);
        break;
    case SB_LINEDOWN:
        ScrollBy(line);
        break;
    case SB_PAGEUP:
        ScrollBy(-page);
        break;
    case SB_PAGEDOWN:
        ScrollBy(page);
        break;
    case SB_TOP:
        SetScrollPosition(0);
        break;
    case SB_BOTTOM:
        SetScrollPosition(MaxScrollPosition(client));
        break;
    case SB_THUMBPOSITION:
    case SB_THUMBTRACK: {
        SCROLLINFO info{sizeof(info)};
        info.fMask = SIF_TRACKPOS;
        ::GetScrollInfo(m_hWnd, SB_VERT, &info);
        SetScrollPosition(info.nTrackPos);
        break;
    }
    default:
        break;
    }
    return 0;
}

LRESULT SettingsPageWindow::OnMouseWheel(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    handled = TRUE;
    const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    const int steps = delta == 0 ? 0 : std::max(1, std::abs(delta) / WHEEL_DELTA);
    ScrollBy(-((delta < 0) ? 1 : -1) * steps * std::max(1, Scale(48)));
    return 0;
}

LRESULT SettingsPageWindow::OnDpiChanged(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    handled = TRUE;
    const UINT dpi = LOWORD(wParam);
    if (dpi > 0) {
        m_dpi = dpi;
    }
    LayoutControls();
    return 0;
}

LRESULT SettingsPageWindow::OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled) {
    if (m_owner != nullptr && ::IsWindow(m_owner)) {
        ::SendMessageW(m_owner, WM_COMMAND, wParam, lParam);
        handled = TRUE;
        return 0;
    }
    handled = FALSE;
    return 0;
}

LRESULT SettingsPageWindow::OnNotify(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled) {
    if (m_owner != nullptr && ::IsWindow(m_owner)) {
        ::SendMessageW(m_owner, WM_NOTIFY, wParam, lParam);
        handled = TRUE;
        return 0;
    }
    handled = FALSE;
    return 0;
}

SettingsWindow::SettingsWindow(Database &database, HWND owner)
    : m_database(database), m_owner(owner) {}

bool SettingsWindow::CreateOrShow() {
    if (m_hWnd != nullptr && ::IsWindow(m_hWnd)) {
        ::SetWindowPos(
            m_hWnd,
            HWND_NOTOPMOST,
            0,
            0,
            0,
            0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
        );
        ShowWindow(::IsIconic(m_hWnd) ? SW_RESTORE : SW_SHOWNORMAL);
        SetForegroundWindow(m_hWnd);
        return true;
    }

    m_destroying = false;
    const HWND window = Create(nullptr);
    if (window == nullptr) {
        return false;
    }

    HMONITOR monitor = ::MonitorFromWindow(m_owner, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        monitor = ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    RECT work_area{0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN)};
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor != nullptr && ::GetMonitorInfoW(monitor, &monitor_info)) {
        work_area = monitor_info.rcWork;
    }

    RECT window_rect{};
    ::GetWindowRect(window, &window_rect);
    const UINT dpi = WindowDpi(window);
    const int window_width = std::max(
        MulDiv(kMinimumSettingsWidth, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI),
        static_cast<int>(window_rect.right - window_rect.left)
    );
    const int window_height = std::max(
        MulDiv(kMinimumSettingsHeight, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI),
        static_cast<int>(window_rect.bottom - window_rect.top)
    );
    const int x = work_area.left + ((work_area.right - work_area.left) - window_width) / 2;
    const int y = work_area.top + ((work_area.bottom - work_area.top) - window_height) / 2;
    ::SetWindowPos(window, HWND_NOTOPMOST, x, y, window_width, window_height, SWP_SHOWWINDOW);
    ::SetForegroundWindow(window);
    return true;
}

void SettingsWindow::DestroyForOwner() {
    m_destroying = true;
    if (m_hWnd != nullptr && ::IsWindow(m_hWnd)) {
        DestroyWindow();
    }
}

void SettingsWindow::AddLayout(
    int page,
    HWND window,
    RECT relative,
    bool stretch_width,
    bool stretch_height,
    bool combo
) {
    if (window == nullptr || page < 0 || page >= kPageCount) {
        return;
    }
    SetControlFont(window);
    m_pages[static_cast<size_t>(page)].AddLayout(window, relative, stretch_width, stretch_height, combo);
}

HWND SettingsWindow::AddStatic(int page, const wchar_t *text, RECT relative, DWORD style) {
    CStatic control;
    const HWND window = control.Create(
        m_pages[static_cast<size_t>(page)].m_hWnd,
        CWindow::rcDefault,
        text,
        WS_CHILD | WS_VISIBLE | style,
        0U,
        0U
    );
    AddLayout(page, window, relative);
    return window;
}

HWND SettingsWindow::AddSectionHeading(int page, const wchar_t *text, RECT relative) {
    const HWND window = AddStatic(page, text, relative, SS_LEFT | SS_NOPREFIX);
    if (window != nullptr && m_sectionFont != nullptr) {
        ::SendMessageW(window, WM_SETFONT, reinterpret_cast<WPARAM>(m_sectionFont), TRUE);
    }
    return window;
}

HWND SettingsWindow::AddButton(int page, const wchar_t *text, int id, RECT relative, DWORD style) {
    CButton control;
    const HWND window = control.Create(
        m_pages[static_cast<size_t>(page)].m_hWnd,
        CWindow::rcDefault,
        text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
        0,
        static_cast<UINT>(id)
    );
    AddLayout(page, window, relative);
    return window;
}

HWND SettingsWindow::AddCheckBox(int page, const wchar_t *text, int id, RECT relative) {
    return AddButton(page, text, id, relative, BS_AUTOCHECKBOX | BS_LEFT | BS_VCENTER);
}

HWND SettingsWindow::AddEdit(int page, int id, RECT relative, DWORD style) {
    CEdit control;
    const HWND window = control.Create(
        m_pages[static_cast<size_t>(page)].m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | style,
        WS_EX_CLIENTEDGE,
        id
    );
    AddLayout(page, window, relative);
    return window;
}

HWND SettingsWindow::AddCombo(int page, int id, RECT relative) {
    CComboBox control;
    // Do not create a drop-down combo with CWindow::rcDefault.  For a
    // CBS_DROPDOWNLIST control, Windows treats the creation height as the
    // height of the expanded combo box.  A zero-sized default rect therefore
    // creates a combo whose drop-down list has no height, even after the
    // visible selection field is laid out later.
    const int initial_width = std::max<int>(
        1,
        m_pages[static_cast<size_t>(page)].Scale(
            static_cast<int>(relative.right - relative.left)
        )
    );
    RECT initial_rect{
        0,
        0,
        initial_width,
        m_pages[static_cast<size_t>(page)].Scale(kComboTotalHeight)
    };
    const HWND window = control.Create(
        m_pages[static_cast<size_t>(page)].m_hWnd,
        initial_rect,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        0,
        id
    );
    AddLayout(page, window, relative, false, false, true);
    return window;
}

HWND SettingsWindow::AddList(int page, int id, RECT relative, bool stretch_height) {
    CListBox control;
    const HWND window = control.Create(
        m_pages[static_cast<size_t>(page)].m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        WS_EX_CLIENTEDGE,
        id
    );
    // Lists use the available page width.  The caller chooses whether the
    // list also owns the available vertical space.
    relative.right = 8;
    AddLayout(page, window, relative, true, stretch_height);
    return window;
}

HWND SettingsWindow::AddHotKey(int page, int id, RECT relative) {
    CHotKeyCtrl control;
    const HWND window = control.Create(
        m_pages[static_cast<size_t>(page)].m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        WS_EX_CLIENTEDGE,
        id
    );
    AddLayout(page, window, relative);
    return window;
}

void SettingsWindow::CreateTabs() {
    m_tabs = GetDlgItem(kTabs);
    SetControlFont(m_tabs.m_hWnd);

    const std::array<const wchar_t *, kPageCount> names = {
        L"通用", L"外观", L"存储", L"忽略", L"置顶", L"高级"
    };
    for (const wchar_t *name : names) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(name);
        m_tabs.InsertItem(m_tabs.GetItemCount(), &item);
    }
}

bool SettingsWindow::CreatePageWindows() {
    if (m_tabs.m_hWnd == nullptr) {
        return false;
    }

    const std::array<bool, kPageCount> scrollable = {false, true, false, true, true, false};
    const std::array<int, kPageCount> heights = {540, 660, 380, 560, 600, 360};
    for (int page = 0; page < kPageCount; ++page) {
        SettingsPageWindow &page_window = m_pages[static_cast<size_t>(page)];
        page_window.Configure(
            m_hWnd,
            scrollable[static_cast<size_t>(page)],
            760,
            heights[static_cast<size_t>(page)]
        );
        const DWORD style = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
            (scrollable[static_cast<size_t>(page)] ? WS_VSCROLL : 0U);
        if (page_window.Create(
                m_tabs.m_hWnd,
                CWindow::rcDefault,
                nullptr,
                style,
                WS_EX_CONTROLPARENT
            ) == nullptr) {
            return false;
        }
        SetControlFont(page_window.m_hWnd);
    }
    return true;
}

void SettingsWindow::CreateGeneralPage() {
    AddSectionHeading(kPageGeneral, L"启动与更新", {16, 8, 740, 32});
    m_gLaunch = AddCheckBox(kPageGeneral, L"登录 Windows 时启动", kGLaunch, {16, 42, 320, 70});
    m_gUpdates = AddCheckBox(kPageGeneral, L"自动检查更新（仅保存选项）", kGUpdates, {16, 76, 380, 104});
    AddButton(kPageGeneral, L"立即检查", kGCheckNow, {400, 74, 500, 104});
    AddStatic(
        kPageGeneral,
        L"Windows 版本没有 Sparkle 更新服务；“立即检查”会打开项目发布页。",
        {16, 112, 740, 145},
        SS_LEFT | SS_NOPREFIX
    );
    AddSectionHeading(kPageGeneral, L"快捷键", {16, 162, 740, 186});
    AddStatic(kPageGeneral, L"打开：", {16, 190, 120, 216});
    m_gOpenHotKey = AddHotKey(kPageGeneral, kGOpenHotKey, {140, 188, 330, 218});
    AddStatic(kPageGeneral, L"置顶：", {16, 226, 120, 252});
    m_gPinHotKey = AddHotKey(kPageGeneral, kGPinHotKey, {140, 224, 330, 254});
    AddStatic(kPageGeneral, L"删除：", {16, 262, 120, 288});
    m_gDeleteHotKey = AddHotKey(kPageGeneral, kGDeleteHotKey, {140, 260, 330, 290});
    AddStatic(kPageGeneral, L"预览：", {16, 298, 120, 324});
    m_gPreviewHotKey = AddHotKey(kPageGeneral, kGPreviewHotKey, {140, 296, 330, 326});

    AddSectionHeading(kPageGeneral, L"行为", {16, 346, 740, 370});
    AddStatic(kPageGeneral, L"搜索模式：", {16, 378, 150, 404});
    m_gSearchMode = AddCombo(kPageGeneral, kGSearchMode, {170, 374, 430, 404});
    AddComboItem(m_gSearchMode, L"精确（不区分大小写）");
    AddComboItem(m_gSearchMode, L"模糊");
    AddComboItem(m_gSearchMode, L"正则表达式");
    AddComboItem(m_gSearchMode, L"混合（精确→正则→模糊）");

    m_gPasteByDefault = AddCheckBox(kPageGeneral, L"选择项目后自动粘贴", kGPasteByDefault, {16, 416, 330, 444});
    m_gRemoveFormatting = AddCheckBox(kPageGeneral, L"默认粘贴为纯文本（去除格式）", kGRemoveFormatting, {16, 450, 390, 478});
    AddButton(kPageGeneral, L"Windows 通知和声音设置", kGNotifications, {16, 494, 270, 526});
}

void SettingsWindow::CreateAppearancePage() {
    AddSectionHeading(kPageAppearance, L"弹出窗口", {16, 8, 740, 32});
    AddStatic(kPageAppearance, L"弹出位置：", {16, 46, 140, 74});
    m_aPopupPosition = AddCombo(kPageAppearance, kAPopupPosition, {150, 42, 360, 72});
    AddComboItem(m_aPopupPosition, L"光标附近");
    AddComboItem(m_aPopupPosition, L"托盘图标附近");
    AddComboItem(m_aPopupPosition, L"目标窗口中心");
    AddComboItem(m_aPopupPosition, L"屏幕中心");
    AddComboItem(m_aPopupPosition, L"上次位置");
    AddStatic(kPageAppearance, L"屏幕：", {390, 46, 445, 74});
    m_aPopupScreen = AddCombo(kPageAppearance, kAPopupScreen, {450, 42, 610, 72});
    AddButton(kPageAppearance, L"重置位置", kAResetPosition, {620, 42, 740, 72});

    AddStatic(kPageAppearance, L"置顶项目位置：", {16, 84, 170, 112});
    m_aPinTo = AddCombo(kPageAppearance, kAPinTo, {190, 80, 350, 110});
    AddComboItem(m_aPinTo, L"顶部");
    AddComboItem(m_aPinTo, L"底部");

    AddSectionHeading(kPageAppearance, L"预览", {16, 132, 740, 156});
    AddStatic(kPageAppearance, L"图片最大高度：", {16, 170, 170, 196});
    m_aImageHeight = AddEdit(kPageAppearance, kAImageHeight, {190, 166, 280, 196}, ES_NUMBER);
    AddStatic(kPageAppearance, L"像素（1–200）", {290, 170, 410, 196});
    m_aOpenPreview = AddCheckBox(kPageAppearance, L"自动打开预览", kAOpenPreview, {16, 208, 280, 236});
    AddStatic(kPageAppearance, L"预览延迟：", {16, 246, 170, 272});
    m_aPreviewDelay = AddEdit(kPageAppearance, kAPreviewDelay, {190, 242, 280, 272}, ES_NUMBER);
    AddStatic(kPageAppearance, L"毫秒（200–100000）", {290, 246, 450, 272});

    AddSectionHeading(kPageAppearance, L"搜索结果显示", {16, 300, 740, 324});
    AddStatic(kPageAppearance, L"搜索匹配样式：", {16, 338, 170, 364});
    m_aHighlight = AddCombo(kPageAppearance, kAHighlight, {190, 334, 350, 364});
    AddComboItem(m_aHighlight, L"颜色");
    AddComboItem(m_aHighlight, L"粗体");
    AddComboItem(m_aHighlight, L"斜体");
    AddComboItem(m_aHighlight, L"下划线");

    AddStatic(kPageAppearance, L"托盘图标：", {16, 378, 140, 404});
    m_aMenuIcon = AddCombo(kPageAppearance, kAMenuIcon, {150, 374, 350, 404});
    AddComboItem(m_aMenuIcon, L"Maccy");
    AddComboItem(m_aMenuIcon, L"剪贴板");
    AddComboItem(m_aMenuIcon, L"剪刀");
    AddComboItem(m_aMenuIcon, L"回形针");
    m_aShowStatus = AddCheckBox(kPageAppearance, L"显示托盘图标", kAShowStatus, {380, 374, 540, 404});
    m_aShowRecent = AddCheckBox(kPageAppearance, L"在托盘提示中显示最近复制内容", kAShowRecent, {16, 416, 400, 444});
    m_aShowSearch = AddCheckBox(kPageAppearance, L"显示搜索框", kAShowSearch, {16, 450, 190, 478});
    m_aSearchVisibility = AddCombo(kPageAppearance, kASearchVisibility, {210, 446, 390, 476});
    AddComboItem(m_aSearchVisibility, L"始终显示");
    AddComboItem(m_aSearchVisibility, L"搜索时显示");
    m_aShowTitle = AddCheckBox(kPageAppearance, L"在搜索框前显示标题", kAShowTitle, {16, 484, 300, 512});
    m_aShowIcons = AddCheckBox(kPageAppearance, L"显示来源程序图标", kAShowIcons, {16, 518, 300, 546});
    m_aShowSwatch = AddCheckBox(kPageAppearance, L"显示十六进制颜色色块", kAShowSwatch, {320, 518, 580, 546});
    m_aShowSpecial = AddCheckBox(kPageAppearance, L"显示换行、制表符和首尾空格符号", kAShowSpecial, {16, 552, 390, 580});
    m_aShowFooter = AddCheckBox(kPageAppearance, L"显示底部状态栏", kAShowFooter, {16, 586, 270, 614});
}

void SettingsWindow::CreateStoragePage() {
    AddSectionHeading(kPageStorage, L"保存类型", {16, 8, 740, 32});
    m_sSaveFiles = AddCheckBox(kPageStorage, L"文件（CF_HDROP）", kSSaveFiles, {24, 44, 280, 72});
    m_sSaveImages = AddCheckBox(kPageStorage, L"图片（DIB/DIBV5）", kSSaveImages, {24, 78, 280, 106});
    m_sSaveText = AddCheckBox(kPageStorage, L"文本（Unicode/HTML/RTF）", kSSaveText, {24, 112, 330, 140});
    AddStatic(
        kPageStorage,
        L"关闭某种类型后，新的剪贴板内容不会保存该类型；已有历史不会被删除。",
        {16, 150, 740, 194},
        SS_LEFT | SS_NOPREFIX
    );
    AddSectionHeading(kPageStorage, L"历史", {16, 220, 740, 244});
    AddStatic(kPageStorage, L"保留历史数量：", {16, 260, 170, 286});
    m_sHistorySize = AddEdit(kPageStorage, kSHistorySize, {190, 256, 280, 286}, ES_NUMBER);
    AddStatic(kPageStorage, L"条（1–999，不含置顶项）", {290, 260, 520, 286});
    AddStatic(kPageStorage, L"排序：", {16, 298, 120, 324});
    m_sSortBy = AddCombo(kPageStorage, kSSortBy, {190, 294, 400, 324});
    AddComboItem(m_sSortBy, L"最近复制时间");
    AddComboItem(m_sSortBy, L"首次复制时间");
    AddComboItem(m_sSortBy, L"复制次数");
    AddStatic(kPageStorage, L"当前数据库大小：", {16, 338, 190, 364});
    m_sStorageSize = AddStatic(kPageStorage, L"", {205, 338, 380, 364});
}

void SettingsWindow::CreateIgnorePage() {
    AddSectionHeading(kPageIgnore, L"忽略规则", {16, 8, 740, 32});
    m_ignoreTabWindow = m_ignoreTabs.Create(
        m_pages[kPageIgnore].m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS,
        0,
        kIgnoreTabs
    );
    AddLayout(kPageIgnore, m_ignoreTabWindow, {8, 42, 8, 78}, true, false);
    const std::array<const wchar_t *, 3> names = {L"应用程序", L"剪贴板格式", L"正则表达式"};
    for (const wchar_t *name : names) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(name);
        m_ignoreTabs.InsertItem(m_ignoreTabs.GetItemCount(), &item);
    }
    m_iList = AddList(kPageIgnore, kIList, {8, 88, 8, 330}, false);
    m_iEdit = AddEdit(kPageIgnore, kIEdit, {8, 346, 430, 376});
    AddButton(kPageIgnore, L"添加", kIAdd, {440, 344, 510, 376});
    AddButton(kPageIgnore, L"浏览…", kIBrowse, {516, 344, 586, 376});
    AddButton(kPageIgnore, L"修改", kIUpdate, {592, 344, 662, 376});
    AddButton(kPageIgnore, L"删除", kIRemove, {668, 344, 738, 376});
    AddButton(kPageIgnore, L"恢复默认", kIReset, {592, 384, 738, 416});
    m_iWhitelist = AddCheckBox(kPageIgnore, L"仅忽略列表中的应用（白名单）", kIWhitelist, {8, 384, 330, 416});
    m_iDescription = AddStatic(kPageIgnore, L"", {8, 434, 752, 530}, SS_LEFT | SS_NOPREFIX);
}

void SettingsWindow::CreatePinsPage() {
    AddSectionHeading(kPagePins, L"置顶项目", {16, 8, 740, 32});
    m_pList = AddList(kPagePins, kPList, {8, 42, 8, 230}, false);
    AddStatic(kPagePins, L"按键：", {8, 250, 100, 276});
    m_pKey = AddEdit(kPagePins, kPKey, {120, 246, 280, 276});
    AddStatic(kPagePins, L"标题：", {8, 286, 100, 312});
    m_pTitle = AddEdit(kPagePins, kPTitle, {120, 282, 752, 312}, ES_AUTOHSCROLL);
    AddStatic(kPagePins, L"内容：", {8, 322, 100, 348});
    m_pContent = AddEdit(kPagePins, kPContent, {120, 318, 752, 470}, ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL);
    m_pContentHint = AddStatic(
        kPagePins,
        L"只能编辑纯文本；图片和文件可以修改按键与标题，但内容保持原样。",
        {8, 480, 752, 520},
        SS_LEFT | SS_NOPREFIX
    );
    AddButton(kPagePins, L"保存置顶项", kPSave, {560, 536, 660, 568});
    AddButton(kPagePins, L"删除置顶项", kPDelete, {670, 536, 752, 568});
}

void SettingsWindow::CreateAdvancedPage() {
    AddSectionHeading(kPageAdvanced, L"复制行为", {16, 8, 740, 32});
    m_xIgnoreEvents = AddCheckBox(kPageAdvanced, L"暂时忽略所有新的复制", kXIgnoreEvents, {16, 44, 330, 72});
    AddStatic(
        kPageAdvanced,
        L"开启后不会记录新的剪贴板变化；“只忽略下一次”会在下一次变化后自动关闭。",
        {16, 82, 740, 126},
        SS_LEFT | SS_NOPREFIX
    );
    m_xIgnoreNext = AddCheckBox(kPageAdvanced, L"只忽略下一次复制", kXIgnoreNext, {16, 140, 300, 168});
    AddStatic(
        kPageAdvanced,
        L"macOS 中可通过 Option 点击菜单图标临时切换；Windows 版提供此处的等价设置。",
        {16, 178, 740, 222},
        SS_LEFT | SS_NOPREFIX
    );
    m_xClearOnQuit = AddCheckBox(kPageAdvanced, L"退出时清空历史", kXClearOnQuit, {16, 256, 300, 284});
    AddStatic(kPageAdvanced, L"只删除未置顶项目。", {320, 256, 520, 284});
    m_xClearClipboard = AddCheckBox(kPageAdvanced, L"同时清空系统剪贴板", kXClearClipboard, {16, 298, 330, 326});
    AddStatic(kPageAdvanced, L"启用后，清空历史也会调用 EmptyClipboard。", {350, 298, 650, 326});
}

RECT SettingsWindow::PageRect() const {
    RECT page{};
    if (m_tabs.m_hWnd == nullptr) {
        return page;
    }
    ::GetClientRect(m_tabs.m_hWnd, &page);
    TabCtrl_AdjustRect(m_tabs.m_hWnd, FALSE, &page);
    page.right = std::max(page.left, page.right);
    page.bottom = std::max(page.top, page.bottom);
    return page;
}

void SettingsWindow::LayoutControls() {
    if (m_tabs.m_hWnd == nullptr) {
        return;
    }

    const RECT page = PageRect();
    const int width = std::max<int>(0, page.right - page.left);
    const int height = std::max<int>(0, page.bottom - page.top);

    for (int page_index = 0; page_index < kPageCount; ++page_index) {
        SettingsPageWindow &page_window = m_pages[static_cast<size_t>(page_index)];
        ::SetWindowPos(
            page_window.m_hWnd,
            nullptr,
            page.left,
            page.top,
            width,
            height,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
        page_window.LayoutControls();
        ::ShowWindow(page_window.m_hWnd, page_index == m_currentPage ? SW_SHOW : SW_HIDE);
    }
}

void SettingsWindow::SetPage(int page) {
    m_currentPage = std::clamp(page, 0, kPageCount - 1);
    if (m_tabs.m_hWnd != nullptr) {
        m_tabs.SetCurSel(m_currentPage);
    }
    if (m_currentPage == kPageIgnore) {
        SetIgnorePage(m_ignorePage);
    }
    LayoutControls();
}

void SettingsWindow::SetIgnorePage(int page) {
    m_ignorePage = std::clamp(page, 0, 2);
    if (m_ignoreTabWindow != nullptr) {
        m_ignoreTabs.SetCurSel(m_ignorePage);
    }

    const bool applications = m_ignorePage == 0;
    const bool formats = m_ignorePage == 1;
    const bool regexps = m_ignorePage == 2;
    const HWND page_window = m_pages[kPageIgnore].m_hWnd;
    ::ShowWindow(::GetDlgItem(page_window, kIBrowse), applications ? SW_SHOW : SW_HIDE);
    ::ShowWindow(::GetDlgItem(page_window, kIReset), formats ? SW_SHOW : SW_HIDE);
    ::ShowWindow(m_iWhitelist, applications ? SW_SHOW : SW_HIDE);

    if (applications) {
        ::SetWindowTextW(m_iDescription, L"忽略来自指定 Windows 应用程序的复制。建议优先使用剪贴板格式规则；列表中保存的是 exe 路径。\r\n开启白名单后，只记录列表中的应用。 ");
    } else if (formats) {
        ::SetWindowTextW(m_iDescription, L"忽略指定的 Windows 剪贴板格式。可填写标准名称（如 CF_UNICODETEXT、CF_HDROP）或注册格式名称。恢复默认会清空 Windows 版自定义列表。");
    } else if (regexps) {
        ::SetWindowTextW(m_iDescription, L"当 Unicode 文本匹配任意有效正则表达式时，不会记录该次复制。无效表达式会被安全忽略。");
    }
    RefreshIgnoreList();
    LayoutControls();
}

void SettingsWindow::LoadGeneralControls() {
    SetCheck(m_gLaunch, m_settings.launch_at_login);
    SetCheck(m_gUpdates, m_settings.check_for_updates);
    SetHotKeyControl(m_gOpenHotKey, m_settings.open_hotkey);
    SetHotKeyControl(m_gPinHotKey, m_settings.pin_hotkey);
    SetHotKeyControl(m_gDeleteHotKey, m_settings.delete_hotkey);
    SetHotKeyControl(m_gPreviewHotKey, m_settings.preview_hotkey);
    SelectCombo(m_gSearchMode, static_cast<int>(m_settings.search_mode));
    SetCheck(m_gPasteByDefault, m_settings.paste_by_default);
    SetCheck(m_gRemoveFormatting, m_settings.remove_formatting_by_default);
}

void SettingsWindow::LoadAppearanceControls() {
    SelectCombo(m_aPopupPosition, static_cast<int>(m_settings.popup_position));
    SendMessageW(m_aPopupScreen, CB_RESETCONTENT, 0, 0);
    const int monitor_count = std::max(1, GetSystemMetrics(SM_CMONITORS));
    AddComboItem(m_aPopupScreen, L"活动屏幕");
    for (int index = 0; index < monitor_count; ++index) {
        const std::wstring name = L"显示器 " + std::to_wstring(index + 1);
        AddComboItem(m_aPopupScreen, name.c_str());
    }
    SelectCombo(m_aPopupScreen, std::clamp(m_settings.popup_screen, 0, monitor_count));
    SelectCombo(m_aPinTo, static_cast<int>(m_settings.pin_to));
    ::SetWindowTextW(m_aImageHeight, std::to_wstring(m_settings.image_max_height).c_str());
    SetCheck(m_aOpenPreview, m_settings.open_preview_automatically);
    ::SetWindowTextW(m_aPreviewDelay, std::to_wstring(m_settings.preview_delay).c_str());
    SelectCombo(m_aHighlight, static_cast<int>(m_settings.highlight_match));

    const std::array<std::wstring, 4> icons = {L"maccy", L"clipboard", L"scissors", L"paperclip"};
    int icon_index = 0;
    for (size_t index = 0; index < icons.size(); ++index) {
        if (icons[index] == m_settings.menu_icon) {
            icon_index = static_cast<int>(index);
            break;
        }
    }
    SelectCombo(m_aMenuIcon, icon_index);
    SetCheck(m_aShowStatus, m_settings.show_in_status_bar);
    SetCheck(m_aShowRecent, m_settings.show_recent_copy_in_menu_bar);
    SetCheck(m_aShowSearch, m_settings.show_search);
    SelectCombo(m_aSearchVisibility, static_cast<int>(m_settings.search_visibility));
    SetCheck(m_aShowSpecial, m_settings.show_special_symbols);
    SetCheck(m_aShowTitle, m_settings.show_title);
    SetCheck(m_aShowIcons, m_settings.show_application_icons);
    SetCheck(m_aShowSwatch, m_settings.show_hex_color_swatch);
    SetCheck(m_aShowFooter, m_settings.show_footer);
    ::EnableWindow(m_aPreviewDelay, m_settings.open_preview_automatically);
}

void SettingsWindow::LoadStorageControls() {
    SetCheck(m_sSaveFiles, m_settings.save_files);
    SetCheck(m_sSaveImages, m_settings.save_images);
    SetCheck(m_sSaveText, m_settings.save_text);
    ::SetWindowTextW(m_sHistorySize, std::to_wstring(m_settings.history_size).c_str());
    SelectCombo(m_sSortBy, m_settings.sort_by);
    ::SetWindowTextW(m_sStorageSize, FormatByteCount(m_database.StorageBytes()).c_str());
}

void SettingsWindow::LoadAdvancedControls() {
    SetCheck(m_xIgnoreEvents, m_settings.ignore_events);
    SetCheck(m_xIgnoreNext, m_settings.ignore_only_next_event);
    SetCheck(m_xClearOnQuit, m_settings.clear_on_quit);
    SetCheck(m_xClearClipboard, m_settings.clear_system_clipboard);
    ::EnableWindow(m_xIgnoreNext, m_settings.ignore_events);
}

void SettingsWindow::LoadControlsFromSettings() {
    m_settings = AppSettings::Load(m_database);
    m_loading = true;
    LoadGeneralControls();
    LoadAppearanceControls();
    LoadStorageControls();
    LoadAdvancedControls();
    m_loading = false;
}

void SettingsWindow::RefreshIgnoreList() {
    if (m_iList == nullptr) {
        return;
    }
    DatabaseList list = DatabaseList::IgnoredApplications;
    if (m_ignorePage == 1) {
        list = DatabaseList::IgnoredFormats;
    } else if (m_ignorePage == 2) {
        list = DatabaseList::IgnoredRegexps;
    }
    const std::vector<std::wstring> values = m_database.GetList(list);
    SendMessageW(m_iList, LB_RESETCONTENT, 0, 0);
    for (const std::wstring &value : values) {
        SendMessageW(m_iList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(value.c_str()));
    }
    SendMessageW(m_iList, LB_SETCURSEL, 0, 0);
    ::SetWindowTextW(m_iEdit, L"");
}

void SettingsWindow::RefreshPinsList() {
    if (m_pList == nullptr) {
        return;
    }
    m_pins = m_database.GetPinnedItems();
    SendMessageW(m_pList, LB_RESETCONTENT, 0, 0);
    for (const ClipboardItem &item : m_pins) {
        const std::wstring display = PinDisplay(item);
        SendMessageW(m_pList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
    }
    m_selectedPinId = 0;
    if (!m_pins.empty()) {
        SendMessageW(m_pList, LB_SETCURSEL, 0, 0);
    }
    LoadSelectedPin();
}

void SettingsWindow::LoadSelectedPin() {
    const LRESULT selected = SendMessageW(m_pList, LB_GETCURSEL, 0, 0);
    if (selected == LB_ERR || static_cast<size_t>(selected) >= m_pins.size()) {
        m_selectedPinId = 0;
        ::SetWindowTextW(m_pKey, L"");
        ::SetWindowTextW(m_pTitle, L"");
        ::SetWindowTextW(m_pContent, L"");
        ::EnableWindow(m_pContent, FALSE);
        m_selectedPinTextEditable = false;
        return;
    }
    const ClipboardItem &item = m_pins[static_cast<size_t>(selected)];
    m_selectedPinId = item.id;
    ::SetWindowTextW(m_pKey, item.pin.c_str());
    ::SetWindowTextW(m_pTitle, item.title.c_str());
    m_selectedPinTextEditable = item.has_text && !item.has_image && !item.has_files;
    if (m_selectedPinTextEditable) {
        ::SetWindowTextW(m_pContent, PinTextContent(item).c_str());
        ::EnableWindow(m_pContent, TRUE);
        ::SetWindowTextW(m_pContentHint, L"可编辑纯文本；保存时会按照 Maccy 的行为丢弃其他格式。 ");
    } else {
        ::SetWindowTextW(m_pContent, L"非文本内容（图片或文件）");
        ::EnableWindow(m_pContent, FALSE);
        ::SetWindowTextW(m_pContentHint, L"只能编辑按键与标题；图片和文件内容不可在 Windows 设置页中直接编辑。 ");
    }
}

void SettingsWindow::SaveCurrentPage(bool notify) {
    if (m_loading) {
        return;
    }

    const AppSettings previous = AppSettings::Load(m_database);
    try {
        switch (m_currentPage) {
        case kPageGeneral:
            m_settings.launch_at_login = IsChecked(m_gLaunch);
            m_settings.check_for_updates = IsChecked(m_gUpdates);
            m_settings.open_hotkey = HotKeyFromControl(m_gOpenHotKey);
            m_settings.pin_hotkey = HotKeyFromControl(m_gPinHotKey);
            m_settings.delete_hotkey = HotKeyFromControl(m_gDeleteHotKey);
            m_settings.preview_hotkey = HotKeyFromControl(m_gPreviewHotKey);
            m_settings.search_mode = static_cast<SearchMode>(ComboSelection(m_gSearchMode));
            m_settings.paste_by_default = IsChecked(m_gPasteByDefault);
            m_settings.remove_formatting_by_default = IsChecked(m_gRemoveFormatting);
            break;
        case kPageAppearance: {
            m_settings.popup_position = static_cast<PopupPosition>(ComboSelection(m_aPopupPosition));
            m_settings.popup_screen = ComboSelection(m_aPopupScreen);
            m_settings.pin_to = static_cast<PinPosition>(ComboSelection(m_aPinTo));
            m_settings.image_max_height = ReadValidatedInteger(
                m_aImageHeight,
                m_settings.image_max_height,
                1,
                200
            );
            m_settings.open_preview_automatically = IsChecked(m_aOpenPreview);
            m_settings.preview_delay = ReadValidatedInteger(
                m_aPreviewDelay,
                m_settings.preview_delay,
                200,
                100000
            );
            m_settings.highlight_match = static_cast<HighlightMatch>(ComboSelection(m_aHighlight));
            const std::array<const wchar_t *, 4> icons = {L"maccy", L"clipboard", L"scissors", L"paperclip"};
            m_settings.menu_icon = icons[static_cast<size_t>(std::clamp(ComboSelection(m_aMenuIcon), 0, 3))];
            m_settings.show_in_status_bar = IsChecked(m_aShowStatus);
            m_settings.show_recent_copy_in_menu_bar = IsChecked(m_aShowRecent);
            m_settings.show_search = IsChecked(m_aShowSearch);
            m_settings.search_visibility = static_cast<SearchVisibility>(ComboSelection(m_aSearchVisibility));
            m_settings.show_special_symbols = IsChecked(m_aShowSpecial);
            m_settings.show_title = IsChecked(m_aShowTitle);
            m_settings.show_application_icons = IsChecked(m_aShowIcons);
            m_settings.show_hex_color_swatch = IsChecked(m_aShowSwatch);
            m_settings.show_footer = IsChecked(m_aShowFooter);
            break;
        }
        case kPageStorage:
            m_settings.save_files = IsChecked(m_sSaveFiles);
            m_settings.save_images = IsChecked(m_sSaveImages);
            m_settings.save_text = IsChecked(m_sSaveText);
            m_settings.history_size = ReadValidatedInteger(
                m_sHistorySize,
                m_settings.history_size,
                1,
                999
            );
            m_settings.sort_by = std::clamp(ComboSelection(m_sSortBy), 0, 2);
            break;
        case kPageIgnore:
            m_settings.ignore_all_apps_except_listed = IsChecked(m_iWhitelist);
            break;
        case kPageAdvanced:
            m_settings.ignore_events = IsChecked(m_xIgnoreEvents);
            m_settings.ignore_only_next_event = IsChecked(m_xIgnoreNext);
            m_settings.clear_on_quit = IsChecked(m_xClearOnQuit);
            m_settings.clear_system_clipboard = IsChecked(m_xClearClipboard);
            break;
        default:
            break;
        }

        m_settings.Save(m_database);
        if (previous.history_size != m_settings.history_size) {
            m_database.TrimUnpinned(m_settings.history_size);
        }
        if (previous.show_special_symbols != m_settings.show_special_symbols) {
            m_database.RegenerateTitles(m_settings.show_special_symbols);
        }
        if (notify) {
            NotifyOwner();
        }
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to save settings", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::NotifyOwner() {
    if (m_owner != nullptr && ::IsWindow(m_owner)) {
        SendMessageW(m_owner, kSettingsChangedMessage, 0, 0);
    }
}

void SettingsWindow::SaveIgnoreList() {
    DatabaseList list = DatabaseList::IgnoredApplications;
    if (m_ignorePage == 1) {
        list = DatabaseList::IgnoredFormats;
    } else if (m_ignorePage == 2) {
        list = DatabaseList::IgnoredRegexps;
    }

    std::vector<std::wstring> values;
    const LRESULT count = SendMessageW(m_iList, LB_GETCOUNT, 0, 0);
    for (LRESULT index = 0; index < count; ++index) {
        const LRESULT length = SendMessageW(m_iList, LB_GETTEXTLEN, index, 0);
        if (length <= 0) {
            continue;
        }
        std::wstring value(static_cast<size_t>(length) + 1, L'\0');
        SendMessageW(m_iList, LB_GETTEXT, index, reinterpret_cast<LPARAM>(value.data()));
        value.resize(static_cast<size_t>(length));
        values.push_back(std::move(value));
    }
    try {
        m_database.ReplaceList(list, values);
        NotifyOwner();
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to save ignore list", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::AddIgnoreValue(bool browse_for_application) {
    if (browse_for_application) {
        std::array<wchar_t, MAX_PATH> path{};
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = m_hWnd;
        dialog.lpstrFilter = L"Windows application (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
        dialog.lpstrFile = path.data();
        dialog.nMaxFile = static_cast<DWORD>(path.size());
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (GetOpenFileNameW(&dialog)) {
            ::SetWindowTextW(m_iEdit, path.data());
        }
    }

    const std::wstring value = ReadWindowText(m_iEdit);
    if (value.empty()) {
        return;
    }
    const LRESULT exists = SendMessageW(m_iList, LB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(value.c_str()));
    if (exists != LB_ERR) {
        return;
    }
    SendMessageW(m_iList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(value.c_str()));
    SaveIgnoreList();
    ::SetWindowTextW(m_iEdit, L"");
}

void SettingsWindow::UpdateIgnoreValue() {
    const LRESULT selected = SendMessageW(m_iList, LB_GETCURSEL, 0, 0);
    const std::wstring value = ReadWindowText(m_iEdit);
    if (selected == LB_ERR || value.empty()) {
        return;
    }
    SendMessageW(m_iList, LB_DELETESTRING, selected, 0);
    SendMessageW(m_iList, LB_INSERTSTRING, selected, reinterpret_cast<LPARAM>(value.c_str()));
    SaveIgnoreList();
    SendMessageW(m_iList, LB_SETCURSEL, selected, 0);
}

void SettingsWindow::RemoveIgnoreValue() {
    const LRESULT selected = SendMessageW(m_iList, LB_GETCURSEL, 0, 0);
    if (selected == LB_ERR) {
        return;
    }
    SendMessageW(m_iList, LB_DELETESTRING, selected, 0);
    SaveIgnoreList();
    ::SetWindowTextW(m_iEdit, L"");
}

void SettingsWindow::ResetIgnoredFormats() {
    try {
        m_database.ResetIgnoredFormats();
        RefreshIgnoreList();
        NotifyOwner();
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to reset ignored formats", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::SaveSelectedPin() {
    if (m_selectedPinId == 0) {
        return;
    }
    const std::wstring key = ReadWindowText(m_pKey);
    const std::wstring title = ReadWindowText(m_pTitle);
    try {
        if (m_selectedPinTextEditable) {
            m_database.UpdatePinnedItem(m_selectedPinId, key, title, ReadWindowText(m_pContent));
        } else {
            m_database.UpdatePinnedMetadata(m_selectedPinId, key, title);
        }
        RefreshPinsList();
        NotifyOwner();
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to save pinned item", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::DeleteSelectedPin() {
    if (m_selectedPinId == 0) {
        return;
    }
    if (::MessageBoxW(m_hWnd, L"删除当前置顶项目？", L"确认", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }
    try {
        m_database.DeleteItem(m_selectedPinId);
        RefreshPinsList();
        NotifyOwner();
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to delete pinned item", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::OpenNotificationsSettings() {
    ShellExecuteW(m_hWnd, L"open", L"ms-settings:notifications", nullptr, nullptr, SW_SHOWNORMAL);
}

void SettingsWindow::CheckForUpdatesNow() {
    ShellExecuteW(
        m_hWnd,
        L"open",
        L"https://github.com/p0deje/Maccy/releases/latest",
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );
}

void SettingsWindow::ResetPopupPosition() {
    m_settings.popup_x = 0;
    m_settings.popup_y = 0;
    try {
        m_settings.Save(m_database);
        NotifyOwner();
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to reset popup position", MB_OK | MB_ICONERROR);
    }
}

LRESULT SettingsWindow::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    // A modeless settings window is deliberately a normal top-level window:
    // it has a title bar, participates in the taskbar and Alt+Tab, and does
    // not inherit the main window's topmost behavior.
    ::SetWindowTextW(m_hWnd, L"剪贴板设置");
    const LONG_PTR extended_style = ::GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE);
    ::SetWindowLongPtrW(m_hWnd, GWL_EXSTYLE, (extended_style & ~WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW);
    ::SetWindowPos(
        m_hWnd,
        HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );

    m_settings = AppSettings::Load(m_database);
    HFONT gui_font = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    LOGFONTW log_font{};
    if (gui_font != nullptr && ::GetObjectW(gui_font, sizeof(log_font), &log_font) == sizeof(log_font)) {
        log_font.lfWeight = FW_SEMIBOLD;
        m_sectionFont = ::CreateFontIndirectW(&log_font);
    }

    CreateTabs();
    if (!CreatePageWindows()) {
        handled = FALSE;
        return FALSE;
    }
    CreateGeneralPage();
    CreateAppearancePage();
    CreateStoragePage();
    CreateIgnorePage();
    CreatePinsPage();
    CreateAdvancedPage();
    LoadControlsFromSettings();
    DlgResize_Init(false, true);
    SetPage(kPageGeneral);
    SetIgnorePage(0);
    return TRUE;
}

LRESULT SettingsWindow::OnSize(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    handled = TRUE;
    if (m_hWnd != nullptr) {
        DlgResize_UpdateLayout(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    }
    LayoutControls();
    return 0;
}

LRESULT SettingsWindow::OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    handled = TRUE;
    auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
    if (info == nullptr) {
        return 0;
    }

    const UINT dpi = WindowDpi(m_hWnd);
    info->ptMinTrackSize.x = std::max<LONG>(
        info->ptMinTrackSize.x,
        MulDiv(kMinimumSettingsWidth, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI)
    );
    info->ptMinTrackSize.y = std::max<LONG>(
        info->ptMinTrackSize.y,
        MulDiv(kMinimumSettingsHeight, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI)
    );
    return 0;
}

LRESULT SettingsWindow::OnDpiChanged(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    handled = TRUE;
    const auto *suggested = reinterpret_cast<const RECT *>(lParam);
    if (suggested != nullptr) {
        ::SetWindowPos(
            m_hWnd,
            nullptr,
            suggested->left,
            suggested->top,
            suggested->right - suggested->left,
            suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }
    SetControlFont(m_tabs.m_hWnd);
    LayoutControls();
    return 0;
}

LRESULT SettingsWindow::OnClose(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    SaveCurrentPage(false);
    if (!m_destroying) {
        ShowWindow(SW_HIDE);
    } else {
        DestroyWindow();
    }
    return 0;
}

LRESULT SettingsWindow::OnCommand(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    handled = TRUE;
    const int id = LOWORD(wParam);
    const int notification = HIWORD(wParam);
    if (m_loading) {
        return 0;
    }

    if (id == kGCheckNow && notification == BN_CLICKED) {
        CheckForUpdatesNow();
        return 0;
    }
    if (id == kGNotifications && notification == BN_CLICKED) {
        OpenNotificationsSettings();
        return 0;
    }
    if (id == kAResetPosition && notification == BN_CLICKED) {
        ResetPopupPosition();
        return 0;
    }
    if (id == kIAdd && notification == BN_CLICKED) {
        AddIgnoreValue(false);
        return 0;
    }
    if (id == kIBrowse && notification == BN_CLICKED) {
        AddIgnoreValue(true);
        return 0;
    }
    if (id == kIUpdate && notification == BN_CLICKED) {
        UpdateIgnoreValue();
        return 0;
    }
    if (id == kIRemove && notification == BN_CLICKED) {
        RemoveIgnoreValue();
        return 0;
    }
    if (id == kIReset && notification == BN_CLICKED) {
        ResetIgnoredFormats();
        return 0;
    }
    if (id == kPList && notification == LBN_SELCHANGE) {
        LoadSelectedPin();
        return 0;
    }
    if (id == kPSave && notification == BN_CLICKED) {
        SaveSelectedPin();
        return 0;
    }
    if (id == kPDelete && notification == BN_CLICKED) {
        DeleteSelectedPin();
        return 0;
    }
    if (id == kIList && notification == LBN_SELCHANGE) {
        const LRESULT selected = SendMessageW(m_iList, LB_GETCURSEL, 0, 0);
        if (selected != LB_ERR) {
            const LRESULT length = SendMessageW(m_iList, LB_GETTEXTLEN, selected, 0);
            if (length > 0) {
                std::wstring value(static_cast<size_t>(length) + 1, L'\0');
                SendMessageW(m_iList, LB_GETTEXT, selected, reinterpret_cast<LPARAM>(value.data()));
                value.resize(static_cast<size_t>(length));
                ::SetWindowTextW(m_iEdit, value.c_str());
            }
        }
        return 0;
    }

    const bool general_change =
        id == kGLaunch || id == kGUpdates || id == kGOpenHotKey || id == kGPinHotKey ||
        id == kGDeleteHotKey || id == kGPreviewHotKey || id == kGSearchMode ||
        id == kGPasteByDefault || id == kGRemoveFormatting;
    const bool appearance_change =
        id == kAPopupPosition || id == kAPopupScreen || id == kAPinTo || id == kAImageHeight ||
        id == kAOpenPreview || id == kAPreviewDelay || id == kAHighlight || id == kAMenuIcon ||
        id == kAShowStatus || id == kAShowRecent || id == kAShowSearch || id == kASearchVisibility ||
        id == kAShowSpecial || id == kAShowTitle || id == kAShowIcons || id == kAShowSwatch ||
        id == kAShowFooter;
    const bool storage_change =
        id == kSSaveFiles || id == kSSaveImages || id == kSSaveText || id == kSHistorySize || id == kSSortBy;
    const bool advanced_change =
        id == kXIgnoreEvents || id == kXIgnoreNext || id == kXClearOnQuit || id == kXClearClipboard;

    const bool combo_changed = notification == CBN_SELCHANGE;
    const bool checkbox_changed = notification == BN_CLICKED;
    const bool hotkey_changed = notification == EN_CHANGE &&
        (id == kGOpenHotKey || id == kGPinHotKey || id == kGDeleteHotKey || id == kGPreviewHotKey);
    const bool numeric_finished = notification == EN_KILLFOCUS &&
        (id == kAImageHeight || id == kAPreviewDelay || id == kSHistorySize);

    if ((general_change && (checkbox_changed || combo_changed || hotkey_changed)) ||
        (appearance_change && (checkbox_changed || combo_changed || numeric_finished)) ||
        (storage_change && (checkbox_changed || combo_changed || numeric_finished)) ||
        (advanced_change && checkbox_changed)) {
        if (id == kAOpenPreview) {
            ::EnableWindow(m_aPreviewDelay, IsChecked(m_aOpenPreview));
        }
        if (id == kXIgnoreEvents) {
            ::EnableWindow(m_xIgnoreNext, IsChecked(m_xIgnoreEvents));
        }
        SaveCurrentPage();
        return 0;
    }

    if (id == kIWhitelist && checkbox_changed) {
        SaveCurrentPage();
        return 0;
    }
    handled = FALSE;
    return 0;
}

LRESULT SettingsWindow::OnNotify(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    handled = TRUE;
    auto *header = reinterpret_cast<NMHDR *>(lParam);
    if (header == nullptr) {
        return 0;
    }
    if (header->idFrom == kTabs && header->code == TCN_SELCHANGE) {
        SaveCurrentPage();
        SetPage(m_tabs.GetCurSel());
        if (m_currentPage == kPageIgnore) {
            SetIgnorePage(m_ignorePage);
        } else if (m_currentPage == kPagePins) {
            RefreshPinsList();
        }
        return 0;
    }
    if (header->idFrom == kIgnoreTabs && header->code == TCN_SELCHANGE) {
        SetIgnorePage(m_ignoreTabs.GetCurSel());
        return 0;
    }
    handled = FALSE;
    return 0;
}

LRESULT SettingsWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    if (m_sectionFont != nullptr) {
        ::DeleteObject(m_sectionFont);
        m_sectionFont = nullptr;
    }
    m_hWnd = nullptr;
    return 0;
}
