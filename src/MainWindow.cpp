#include "MainWindow.h"
#include "Constants.h"
#include "PinKeys.h"
#include "SettingsWindow.h"

#include <algorithm>
#include <array>

// External global module instance
extern CAppModule _Module;

namespace {

constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayCommandShow = 1001;
constexpr UINT kTrayCommandSettings = 1002;
constexpr UINT kTrayCommandClear = 1003;
constexpr UINT kTrayCommandIgnore = 1004;
constexpr UINT kTrayCommandExit = 1005;
constexpr UINT kSearchDebounceMilliseconds = 200;

constexpr int kSearchControlId = IDC_HISTORY_SEARCH;
constexpr int kHistoryListControlId = IDC_HISTORY_LIST;

constexpr int kMinimumPopupWidth = 320;
constexpr int kMaximumPopupWidth = 1600;
constexpr int kMinimumPopupHeight = 150;
constexpr int kMaximumPopupHeight = 1200;
constexpr int kHistorySearchHeight = 23;
constexpr int kHistoryItemHeight = 22;
constexpr int kHistoryWindowMargin = 5;
constexpr int kHistorySearchGap = 6;
constexpr int kHistorySearchIconWidth = 24;
constexpr int kHistorySearchClearWidth = 20;
constexpr int kHistoryPreviewWidth = 23;
constexpr int kHistoryHeaderGap = 6;
constexpr int kHistoryFooterHeight = 22;
constexpr int kHistoryFooterGap = 6;
constexpr int kHistorySectionGap = 6;
constexpr int kHistoryFooterCount = 4;
constexpr int kResizeBorder = 8;

constexpr wchar_t kControlOwnerProperty[] = L"maccyMainWindow";

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

HICON TrayIconForName(std::wstring_view name) {
    if (name == L"clipboard") {
        return LoadIconW(nullptr, IDI_INFORMATION);
    }
    if (name == L"scissors") {
        return LoadIconW(nullptr, IDI_WARNING);
    }
    if (name == L"paperclip") {
        return LoadIconW(nullptr, IDI_QUESTION);
    }
    return LoadIconW(nullptr, IDI_APPLICATION);
}

bool IsShiftKey(WPARAM key) {
    return key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT;
}

// Resolve paste action based on modifier keys
std::pair<bool, bool> ResolvePasteAction(bool paste_default, bool plain_default,
                                         bool ctrl, bool alt, bool shift) {
    bool paste = paste_default;
    bool plain = plain_default;

    if (ctrl && !alt && !shift) {
        paste = paste_default;
        plain = plain_default;
    } else if (alt && !ctrl && !shift) {
        paste = !paste_default;
        plain = plain_default;
    } else if (ctrl && shift && !alt) {
        paste = paste_default;
        plain = !plain_default;
    } else if (alt && shift && !ctrl) {
        paste = !paste_default;
        plain = !plain_default;
    } else {
        paste = paste_default;
        plain = plain_default;
    }

    return {paste, plain};
}

} // namespace

MainWindow::MainWindow(Database& database, bool isolated)
    : m_database(database),
      m_settings(AppSettings::Load(database)),
      m_clipboardMonitor(database, m_settings),
      m_historyRenderer(m_settings),
      m_keyboardHandler(m_settings),
      m_isolated(isolated) {}

MainWindow::~MainWindow() = default;

LRESULT CALLBACK MainWindow::SearchWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* owner = reinterpret_cast<MainWindow*>(GetPropW(window, kControlOwnerProperty));
    if (!owner) return ::DefWindowProcW(window, message, wParam, lParam);
    owner->RequestFooterUpdateForKeyMessage(message, wParam);
    if (message == WM_IME_STARTCOMPOSITION) owner->m_keyboardHandler.SetImeComposing(true);
    if (message == WM_IME_ENDCOMPOSITION) owner->m_keyboardHandler.SetImeComposing(false);
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
        !owner->m_keyboardHandler.IsComposing(window) &&
        owner->m_keyboardHandler.HandlePopupKey(wParam, owner->m_search, owner->m_items)) {
        return 0;
    }
    return CallWindowProcW(owner->m_originalSearchProc, window, message, wParam, lParam);
}

LRESULT CALLBACK MainWindow::HistoryListWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* owner = reinterpret_cast<MainWindow*>(GetPropW(window, kControlOwnerProperty));
    if (!owner) return ::DefWindowProcW(window, message, wParam, lParam);
    owner->RequestFooterUpdateForKeyMessage(message, wParam);
    if (message == WM_MOUSEMOVE) {
        owner->m_keyboardHandler.OnHistoryMouseMove(
            window,
            POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
            owner->m_items,
            owner->m_historyList,
            owner->m_pinsList,
            owner->m_popupVisible
        );
    }
    if (message == WM_MOUSELEAVE) owner->m_keyboardHandler.OnHistoryMouseLeave();
    if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
        if (owner->m_keyboardHandler.HandlePopupKey(wParam, owner->m_search, owner->m_items)) return 0;
    }
    if (message == WM_CHAR && wParam >= 0x20 && wParam != 0x7f) {
        owner->m_keyboardHandler.TypeToSearch(wParam, owner->m_search);
        return 0;
    }
    if (message == WM_LBUTTONUP) {
        const int index = owner->m_keyboardHandler.HistoryItemAtPoint(
            window,
            {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
            owner->m_historyList,
            owner->m_pinsList,
            owner->m_items);
        if (index >= 0) owner->PasteItem(index);
        return 0;
    }
    const LRESULT result = CallWindowProcW(owner->m_originalHistoryListProc, window, message, wParam, lParam);
    if (message == WM_MOUSEWHEEL || message == WM_VSCROLL) {
        owner->m_keyboardHandler.UpdateHistoryHoverFromCursor(owner->m_items,
            owner->m_historyList, owner->m_pinsList, owner->m_popupVisible);
    }
    return result;
}

LRESULT CALLBACK MainWindow::MenuControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR, DWORD_PTR data) {
    auto* owner = reinterpret_cast<MainWindow*>(data);
    owner->RequestFooterUpdateForKeyMessage(message, wParam);
    if (message == WM_MOUSEMOVE) {
        if (!owner->m_keyboardHandler.MouseCanSelect()) return DefSubclassProc(window, message, wParam, lParam);
        const auto buttons = owner->FooterButtons();
        const auto it = std::find(buttons.begin(), buttons.end(), window);
        if (it != buttons.end()) {
            owner->m_keyboardHandler.SetActiveFooter(static_cast<int>(it - buttons.begin()),
                owner->m_items);
        }
    }
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && wParam == VK_RETURN &&
        (::GetDlgCtrlID(window) == IDC_HISTORY_PREVIEW || ::GetDlgCtrlID(window) == IDC_HISTORY_SEARCH_CLEAR)) {
        SendMessageW(owner->m_hWnd, WM_COMMAND, MAKEWPARAM(::GetDlgCtrlID(window), BN_CLICKED),
            reinterpret_cast<LPARAM>(window));
        return 0;
    }
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
        owner->m_keyboardHandler.HandlePopupKey(wParam, owner->m_search, owner->m_items)) {
        return 0;
    }
    if (message == WM_CHAR && wParam >= 0x20 && wParam != 0x7f) {
        owner->m_keyboardHandler.TypeToSearch(wParam, owner->m_search);
        return 0;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

std::array<HWND, 4> MainWindow::FooterButtons() const {
    return {m_footerClear, m_footerSettings, m_footerAbout, m_footerExit};
}

void MainWindow::RedrawHistoryLists() {
    // A resized owner-draw list can retain pixels from the previous item
    // width. Repaint the complete visible list after its final geometry is set.
    for (HWND list : {m_historyList, m_pinsList}) {
        if (list == nullptr || !::IsWindowVisible(list)) {
            continue;
        }
        ::RedrawWindow(list, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
    }
}

void MainWindow::RedrawFooterButtons() {
    for (HWND button : FooterButtons()) {
        if (button == nullptr || !::IsWindowVisible(button)) {
            continue;
        }
        ::RedrawWindow(button, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
    }
}

bool MainWindow::BindControls() {
    m_search = ::GetDlgItem(m_hWnd, kSearchControlId);
    m_historyList = ::GetDlgItem(m_hWnd, kHistoryListControlId);
    m_pinsList = ::GetDlgItem(m_hWnd, IDC_HISTORY_PINS);
    m_searchClear = ::GetDlgItem(m_hWnd, IDC_HISTORY_SEARCH_CLEAR);
    m_previewToggle = ::GetDlgItem(m_hWnd, IDC_HISTORY_PREVIEW);
    m_footerClear = ::GetDlgItem(m_hWnd, IDC_HISTORY_CLEAR);
    m_footerSettings = ::GetDlgItem(m_hWnd, IDC_HISTORY_SETTINGS);
    m_footerAbout = ::GetDlgItem(m_hWnd, IDC_HISTORY_ABOUT);
    m_footerExit = ::GetDlgItem(m_hWnd, IDC_HISTORY_EXIT);

    if (m_search == nullptr || m_historyList == nullptr ||
        m_footerClear == nullptr || m_footerSettings == nullptr ||
        m_footerAbout == nullptr || m_footerExit == nullptr) {
        return false;
    }

    SetPropW(m_search, kControlOwnerProperty, reinterpret_cast<HANDLE>(this));
    SetPropW(m_historyList, kControlOwnerProperty, reinterpret_cast<HANDLE>(this));
    m_originalSearchProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
        m_search, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&MainWindow::SearchWindowProc)));
    m_originalHistoryListProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
        m_historyList, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&MainWindow::HistoryListWindowProc)));

    ApplyHistoryFonts();

    SetPropW(m_pinsList, kControlOwnerProperty, reinterpret_cast<HANDLE>(this));
    m_originalPinsProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(m_pinsList, GWLP_WNDPROC,
        reinterpret_cast<LONG_PTR>(&MainWindow::HistoryListWindowProc)));

    for (HWND button : FooterButtons()) {
        SetWindowSubclass(button, MenuControlProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }
    for (HWND button : {m_searchClear, m_previewToggle}) {
        SetWindowSubclass(button, MenuControlProc, 1, reinterpret_cast<DWORD_PTR>(this));
    }

    // Remove borders
    for (HWND control : {m_search, m_historyList, m_pinsList}) {
        ::SetWindowLongPtrW(control, GWL_STYLE, ::GetWindowLongPtrW(control, GWL_STYLE) & ~WS_BORDER);
        ::SetWindowLongPtrW(control, GWL_EXSTYLE, ::GetWindowLongPtrW(control, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
        ::SetWindowPos(control, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    ::SetWindowTextW(m_previewToggle, L"预览");
    ::SetWindowTextW(m_searchClear, L"清除搜索");
    m_previewTip = L"显示或隐藏预览（" + HotKeyToText(m_settings.preview_hotkey) + L"）";

    m_tooltips = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr,
        _Module.GetModuleInstance(), nullptr);
    for (HWND control : {m_searchClear, m_previewToggle}) {
        TOOLINFOW info{sizeof(info)};
        info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
        info.hwnd = m_hWnd; info.uId = reinterpret_cast<UINT_PTR>(control);
        info.lpszText = const_cast<wchar_t*>(control == m_searchClear ?
            L"清除搜索（Ctrl+U）" : m_previewTip.c_str());
        SendMessageW(m_tooltips, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&info));
    }

    LONG_PTR search_style = ::GetWindowLongPtrW(m_search, GWL_STYLE);
    search_style &= ~static_cast<LONG_PTR>(ES_MULTILINE);
    search_style |= ES_AUTOHSCROLL;
    ::SetWindowLongPtrW(m_search, GWL_STYLE, search_style);
    SendMessageW(m_search, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"搜索剪贴板内容…"));

    return true;
}

void MainWindow::ApplyHistoryFonts() {
    SendMessageW(::GetDlgItem(m_hWnd, IDC_HISTORY_TITLE), WM_SETFONT,
        reinterpret_cast<WPARAM>(m_historyRenderer.GetSmallFont()), TRUE);
    const HFONT font = m_historyRenderer.GetNormalFont();
    for (HWND control : {m_search, m_historyList, m_pinsList, m_footerClear, m_footerSettings,
                         m_footerAbout, m_footerExit, m_searchClear, m_previewToggle}) {
        if (control != nullptr) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }
}

void MainWindow::LayoutHistoryControls() {
    if (!m_search || !m_historyList || !::IsWindow(m_hWnd)) return;
    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    const int margin = kHistoryWindowMargin;
    const int width = std::max(1L, client.right - 2 * margin);
    const bool header = ::IsWindowVisible(m_search) != FALSE;
    const int headerHeight = header ? kHistorySearchHeight : 0;
    int titleWidth = 0;

    if (header && m_settings.show_title) {
        HDC dc = ::GetDC(m_hWnd);
        if (dc != nullptr) {
            const HFONT font = m_historyRenderer.GetSmallFont();
            const HGDIOBJ oldFont = font != nullptr ? ::SelectObject(dc, font) : nullptr;
            SIZE textSize{};
            if (::GetTextExtentPoint32W(dc, L"maccy", 5, &textSize)) {
                titleWidth = textSize.cx + 8;
            }
            if (oldFont != nullptr) ::SelectObject(dc, oldFont);
            ::ReleaseDC(m_hWnd, dc);
        }
    }

    const bool showTitle = header && titleWidth > 0;
    const RECT titleRect{margin, margin, margin + titleWidth, margin + headerHeight};
    const int previewLeft = margin + width - kHistoryPreviewWidth;
    const int searchLeft = margin + titleWidth + (titleWidth ? kHistoryHeaderGap : 0);
    const int searchRight = previewLeft - kHistoryHeaderGap;
    const RECT searchRect{searchLeft, margin, searchRight, margin + headerHeight};

    const int top = margin + (header ? headerHeight + kHistorySearchGap : 0);
    const int footerHeight = m_settings.show_footer
        ? kHistoryFooterGap + kHistoryFooterHeight * kHistoryFooterCount
        : 0;
    const int bottom = std::max(
        top + 1,
        static_cast<int>(client.bottom) - margin - footerHeight
    );
    const int available = std::max(1, bottom - top);
    const int pinCount = static_cast<int>(SendMessageW(m_pinsList, LB_GETCOUNT, 0, 0));
    const int historyCount = static_cast<int>(SendMessageW(m_historyList, LB_GETCOUNT, 0, 0));
    const bool havePins = pinCount > 0;
    const bool haveHistory = historyCount > 0;
    const int gap = havePins && haveHistory ? kHistorySectionGap : 0;
    const int requestedPinsHeight = pinCount * kHistoryItemHeight;
    const int pinsHeight = havePins
        ? std::min(requestedPinsHeight, haveHistory
            ? std::max(1, available - gap - kHistoryItemHeight)
            : available)
        : 0;
    const int historyHeight = haveHistory ? std::max(1, available - pinsHeight - gap) : 1;
    const bool pinsAtBottom = m_settings.pin_to == PinPosition::Bottom;
    const int pinTop = pinsAtBottom && haveHistory ? top + historyHeight + gap : top;
    const int historyTop = !pinsAtBottom && havePins ? top + pinsHeight + gap : top;

    m_titleRect = titleRect;
    m_searchRect = searchRect;
    const HWND title = ::GetDlgItem(m_hWnd, IDC_HISTORY_TITLE);
    ::SetWindowPos(title, nullptr, titleRect.left, titleRect.top,
        titleRect.right - titleRect.left, titleRect.bottom - titleRect.top,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ::ShowWindow(title, showTitle ? SW_SHOW : SW_HIDE);

    if (header) {
        const int searchEditLeft = searchLeft + kHistorySearchIconWidth;
        const int searchEditRight = std::max(
            searchEditLeft + 1,
            searchRight - kHistorySearchClearWidth
        );
        ::SetWindowPos(m_search, nullptr, searchEditLeft, margin,
            searchEditRight - searchEditLeft, kHistorySearchHeight,
            SWP_NOZORDER | SWP_NOACTIVATE);
        ::SetWindowPos(m_searchClear, nullptr, searchRight - kHistorySearchClearWidth, margin,
            kHistorySearchClearWidth, kHistorySearchHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        ::SetWindowPos(m_previewToggle, nullptr, previewLeft, margin,
            kHistoryPreviewWidth, kHistorySearchHeight, SWP_NOZORDER | SWP_NOACTIVATE);
    } else {
        for (HWND control : {m_search, m_searchClear, m_previewToggle}) {
            ::SetWindowPos(control, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
        }
    }

    ::ShowWindow(m_searchClear, header && !ReadWindowText(m_search).empty() ? SW_SHOW : SW_HIDE);
    ::ShowWindow(m_previewToggle, header ? SW_SHOW : SW_HIDE);

    ::SetWindowPos(m_pinsList, nullptr, margin, pinTop, width, pinsHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ::ShowWindow(m_pinsList, havePins ? SW_SHOW : SW_HIDE);
    ::SetWindowPos(m_historyList, nullptr, margin, historyTop, width, historyHeight,
        SWP_NOZORDER | SWP_NOACTIVATE);
    ::ShowWindow(m_historyList, haveHistory || !havePins ? SW_SHOW : SW_HIDE);

    m_pinSeparatorY = gap ? (pinsAtBottom ? pinTop - gap / 2 : historyTop - gap / 2) : -1;
    m_footerSeparatorY = m_settings.show_footer ? bottom + 5 : -1;

    const auto buttons = FooterButtons();
    for (int i = 0; i < 4; ++i) {
        ::SetWindowPos(buttons[i], nullptr, margin,
                       bottom + kHistoryFooterGap + i * kHistoryFooterHeight,
                       width, kHistoryFooterHeight,
                       SWP_NOZORDER | SWP_NOACTIVATE);
        ::ShowWindow(buttons[i], m_settings.show_footer ? SW_SHOW : SW_HIDE);
    }

    UpdateFooterControls();
    RedrawHistoryLists();
    RedrawFooterButtons();
    ::InvalidateRect(m_hWnd, nullptr, TRUE);
}

void MainWindow::ApplyHistoryVisibility() {
    if (m_search == nullptr) return;
    const bool hasQuery = !ReadWindowText(m_search).empty();
    const bool show_search = m_settings.show_search &&
        (m_settings.search_visibility == SearchVisibility::Always || hasQuery);
    SetHistorySearchVisible(show_search);
    LayoutHistoryControls();
}

void MainWindow::SetHistorySearchVisible(bool visible) {
    if (m_search == nullptr || !::IsWindow(m_search)) return;
    ::ShowWindow(m_search, visible ? SW_SHOW : SW_HIDE);
}

void MainWindow::UpdateFooterControls() {
    if (!m_footerClear) return;
    const bool all = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
    const std::wstring caption = all ? L"清空全部历史" : L"清空历史";
    if (ReadWindowText(m_footerClear) != caption) {
        ::SetWindowTextW(m_footerClear, caption.c_str());
    }
}

void MainWindow::RequestFooterUpdateForKeyMessage(UINT message, WPARAM key) {
    const bool key_down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
    const bool key_up = message == WM_KEYUP || message == WM_SYSKEYUP;
    if (key_up || (key_down && IsShiftKey(key))) {
        RequestUiUpdate(AppConstants::UiUpdate::kFooter);
    }
}

void MainWindow::RestoreControlSubclass(HWND control, WNDPROC original) {
    if (control == nullptr || original == nullptr || !::IsWindow(control)) return;
    ::SetWindowLongPtrW(control, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
    RemovePropW(control, kControlOwnerProperty);
}

void MainWindow::RestoreControlSubclasses() {
    RestoreControlSubclass(m_search, m_originalSearchProc);
    RestoreControlSubclass(m_historyList, m_originalHistoryListProc);
    RestoreControlSubclass(m_pinsList, m_originalPinsProc);
    m_originalSearchProc = nullptr;
    m_originalHistoryListProc = nullptr;
}

bool MainWindow::IsOurWindow(HWND window) const {
    if (window == nullptr) return false;
    return window == m_hWnd || window == m_search || window == m_historyList ||
        ::IsChild(m_hWnd, window) || m_previewWindow.ContainsWindow(window) ||
        (m_settingsWindow != nullptr && window == m_settingsWindow->Window());
}

int MainWindow::PopupWidth() const {
    return std::clamp(m_settings.window_width, kMinimumPopupWidth, kMaximumPopupWidth);
}

int MainWindow::PopupHeight() const {
    return std::clamp(m_settings.window_height, kMinimumPopupHeight, kMaximumPopupHeight);
}

void MainWindow::PositionOnMonitor(HMONITOR monitor, bool center) {
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }
    const RECT& work_area = monitor_info.rcWork;
    int x = work_area.left;
    int y = work_area.top;
    if (center) {
        x = work_area.left + ((work_area.right - work_area.left) - PopupWidth()) / 2;
        y = work_area.top + ((work_area.bottom - work_area.top) - PopupHeight()) / 2;
    }
    const LONG max_x = std::max<LONG>(work_area.left, work_area.right - PopupWidth());
    const LONG max_y = std::max<LONG>(work_area.top, work_area.bottom - PopupHeight());
    x = std::clamp(x, static_cast<int>(work_area.left), static_cast<int>(max_x));
    y = std::clamp(y, static_cast<int>(work_area.top), static_cast<int>(max_y));
    ::SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, PopupWidth(), PopupHeight(), SWP_NOACTIVATE);
}

HMONITOR MainWindow::SelectedMonitor() const {
    POINT cursor{};
    GetCursorPos(&cursor);
    if (m_settings.popup_screen == 0) {
        return MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
    }

    struct MonitorSearch {
        int wanted = 0;
        int current = 0;
        HMONITOR result = nullptr;
    } search{m_settings.popup_screen, 0, nullptr};
    EnumDisplayMonitors(
        nullptr,
        nullptr,
        [](HMONITOR monitor, HDC, LPRECT, LPARAM data) -> BOOL {
            auto* search = reinterpret_cast<MonitorSearch*>(data);
            ++search->current;
            if (search->current == search->wanted) {
                search->result = monitor;
                return FALSE;
            }
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&search)
    );
    return search.result != nullptr
        ? search.result
        : MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
}

void MainWindow::PositionPopup(PopupPosition popup_position) {
    POINT cursor{};
    GetCursorPos(&cursor);
    RECT target{};
    const bool has_target = m_clipboardMonitor.GetTargetWindow() != nullptr
        && ::IsWindow(m_clipboardMonitor.GetTargetWindow())
        && ::GetWindowRect(m_clipboardMonitor.GetTargetWindow(), &target) == TRUE;
    const HMONITOR monitor = SelectedMonitor();
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }
    const RECT& work_area = monitor_info.rcWork;
    int x = cursor.x;
    int y = cursor.y - PopupHeight();

    switch (popup_position) {
    case PopupPosition::WindowCenter: {
        if (has_target) {
            x = target.left + ((target.right - target.left) - PopupWidth()) / 2;
            y = target.top + ((target.bottom - target.top) - PopupHeight()) / 2;
        } else {
            x = work_area.left + ((work_area.right - work_area.left) - PopupWidth()) / 2;
            y = work_area.top + ((work_area.bottom - work_area.top) - PopupHeight()) / 2;
        }
        break;
    }
    case PopupPosition::ScreenCenter:
        x = work_area.left + ((work_area.right - work_area.left) - PopupWidth()) / 2;
        y = work_area.top + ((work_area.bottom - work_area.top) - PopupHeight()) / 2;
        break;
    case PopupPosition::LastPosition:
        if (m_settings.popup_x != 0 || m_settings.popup_y != 0) {
            x = m_settings.popup_x;
            y = m_settings.popup_y;
        } else {
            x = work_area.left + ((work_area.right - work_area.left) - PopupWidth()) / 2;
            y = work_area.top + ((work_area.bottom - work_area.top) - PopupHeight()) / 2;
        }
        break;
    case PopupPosition::StatusItem: {
        NOTIFYICONIDENTIFIER identifier{};
        identifier.cbSize = sizeof(identifier);
        identifier.hWnd = m_hWnd;
        identifier.uID = kTrayIconId;
        RECT tray_rect{};
        if (m_trayIconAdded && SUCCEEDED(Shell_NotifyIconGetRect(&identifier, &tray_rect))) {
            x = tray_rect.left + ((tray_rect.right - tray_rect.left) - PopupWidth()) / 2;
            y = tray_rect.top - PopupHeight() - 6;
        }
        break;
    }
    case PopupPosition::Cursor:
    default:
        // Anchor each axis to the cursor like a Windows context menu. The
        // popup's top-left corner is the preferred anchor; flip only the
        // axis whose preferred side has no room in the work area.
        x = cursor.x <= work_area.right - PopupWidth()
            ? cursor.x
            : cursor.x - PopupWidth();
        y = cursor.y <= work_area.bottom - PopupHeight()
            ? cursor.y
            : cursor.y - PopupHeight();
        break;
    }

    const LONG max_x = std::max<LONG>(work_area.left, work_area.right - PopupWidth());
    const LONG max_y = std::max<LONG>(work_area.top, work_area.bottom - PopupHeight());
    x = std::clamp(x, static_cast<int>(work_area.left), static_cast<int>(max_x));
    y = std::clamp(y, static_cast<int>(work_area.top), static_cast<int>(max_y));
    ::SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, PopupWidth(), PopupHeight(), SWP_NOACTIVATE);
}

void MainWindow::PositionPreviewWindow() {
    const HWND preview = m_previewWindow.Window();
    if (preview == nullptr || !::IsWindow(preview)) {
        return;
    }

    RECT main_rect{};
    RECT preview_rect{};
    if (!::GetWindowRect(m_hWnd, &main_rect) ||
        !::GetWindowRect(preview, &preview_rect)) {
        return;
    }

    const int width = preview_rect.right - preview_rect.left;
    const int height = preview_rect.bottom - preview_rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }
    HMONITOR monitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor == nullptr || !::GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    const RECT& work_area = monitor_info.rcWork;
    constexpr int gap = 8;
    int x = main_rect.right + gap;
    int y = main_rect.top;
    const bool fits_right = main_rect.right + gap + width <= work_area.right;
    const bool fits_left = main_rect.left - gap - width >= work_area.left;
    if (fits_right) {
        x = main_rect.right + gap;
    } else if (fits_left) {
        x = main_rect.left - width - gap;
    } else {
        const bool fits_above = main_rect.top - gap - height >= work_area.top;
        const bool fits_below = main_rect.bottom + gap + height <= work_area.bottom;
        x = std::clamp(
            static_cast<int>(main_rect.left),
            static_cast<int>(work_area.left),
            std::max<int>(work_area.left, work_area.right - width)
        );
        if (fits_above) {
            y = main_rect.top - height - gap;
        } else if (fits_below) {
            y = main_rect.bottom + gap;
        }
    }
    const int max_x = std::max<int>(work_area.left, work_area.right - width);
    const int max_y = std::max<int>(work_area.top, work_area.bottom - height);
    x = std::clamp(x, static_cast<int>(work_area.left), max_x);
    y = std::clamp(y, static_cast<int>(work_area.top), max_y);

    ::SetWindowPos(
        preview,
        HWND_TOPMOST,
        x,
        y,
        0,
        0,
        SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW
    );
}

void MainWindow::RefreshHistory(std::wstring_view query) {
    try {
        const bool sameQuery = query == m_searchQuery;
        const sqlite3_int64 previous = sameQuery ? m_keyboardHandler.GetActiveItemId() : 0;
        const int previousIndex = m_keyboardHandler.GetActiveItemIndex();
        const bool previewOpen = m_previewWindow.IsVisible();
        const std::wstring ownedQuery(query);
        m_items = m_database.SearchHistory(
            ownedQuery,
            static_cast<int>(m_settings.search_mode),
            m_settings.sort_by,
            m_settings.pin_to == PinPosition::Bottom
        );

        KillTimer(AppConstants::Timer::kPreview);
        m_previewCandidateId = 0;
        m_keyboardHandler.ClearHistoryHover();
        m_searchQuery = ownedQuery;

        m_loadingList = true;
        for (HWND list : {m_historyList, m_pinsList}) {
            if (list == nullptr) {
                continue;
            }
            SendMessageW(list, WM_SETREDRAW, FALSE, 0);
            SendMessageW(list, LB_RESETCONTENT, 0, 0);
        }

        int selected = -1;
        for (size_t index = 0; index < m_items.size(); ++index) {
            const auto& item = m_items[index];
            const HWND list = item.pinned ? m_pinsList : m_historyList;
            if (list == nullptr) {
                continue;
            }
            const std::wstring display = m_historyRenderer.DisplayText(item);
            const LRESULT row = SendMessageW(
                list,
                LB_ADDSTRING,
                0,
                reinterpret_cast<LPARAM>(display.c_str())
            );
            if (row != LB_ERR) {
                SendMessageW(list, LB_SETITEMDATA, row, static_cast<LPARAM>(index));
            }
            if (item.id == previous) {
                selected = static_cast<int>(index);
            }
        }

        if (selected < 0 && !m_items.empty()) {
            if (previous != 0 && sameQuery) {
                selected = std::clamp(previousIndex, 0, static_cast<int>(m_items.size()) - 1);
            } else {
                selected = 0;
                if (ownedQuery.empty()) {
                    const auto it = std::find_if(
                        m_items.begin(),
                        m_items.end(),
                        [](const auto& item) { return !item.pinned; }
                    );
                    if (it != m_items.end()) {
                        selected = static_cast<int>(it - m_items.begin());
                    }
                }
            }
        }
        if (selected >= 0) {
            m_keyboardHandler.SetActiveHistoryItem(
                selected,
                m_items,
                m_historyList,
                m_pinsList
            );
        }

        m_loadingList = false;
        for (HWND list : {m_historyList, m_pinsList}) {
            if (list != nullptr) {
                SendMessageW(list, WM_SETREDRAW, TRUE, 0);
            }
        }
        m_selectedItemId = m_keyboardHandler.GetActiveItemId();
        RedrawHistoryLists();
        if (previewOpen && m_keyboardHandler.GetActiveItemId()) {
            ShowPreviewForItem(m_keyboardHandler.GetActiveItemId());
        } else if (!m_keyboardHandler.GetActiveItemId()) {
            HidePreview();
        }
    } catch (const std::exception& error) {
        m_loadingList = false;
        for (HWND list : {m_historyList, m_pinsList}) {
            if (list != nullptr) {
                SendMessageW(list, WM_SETREDRAW, TRUE, 0);
            }
        }
        OutputDebugStringA(error.what());
    }
}

void MainWindow::ScheduleSearch() {
    KillTimer(AppConstants::Timer::kSearch);
    ::SetTimer(m_hWnd, AppConstants::Timer::kSearch, kSearchDebounceMilliseconds, nullptr);
}

void MainWindow::SchedulePreviewForItem(sqlite3_int64 item_id) {
    KillTimer(AppConstants::Timer::kPreview);
    m_previewCandidateId = 0;
    if (!m_popupVisible || m_previewSuppressed || item_id == 0) {
        return;
    }
    if (m_previewWindow.IsVisible()) {
        if (m_previewItemId != item_id) {
            ShowPreviewForItem(item_id);
        }
        return;
    }
    if (!m_settings.open_preview_automatically) {
        return;
    }
    m_previewCandidateId = item_id;
    ::SetTimer(m_hWnd, AppConstants::Timer::kPreview, static_cast<UINT>(m_settings.preview_delay), nullptr);
}

void MainWindow::ShowPreviewForItem(sqlite3_int64 item_id) {
    if (item_id <= 0) {
        HidePreview();
        return;
    }
    KillTimer(AppConstants::Timer::kPreview);
    try {
        const auto item = m_database.GetItem(item_id, true);
        if (!item) {
            HidePreview();
            return;
        }
        m_previewWindow.SetItem(*item);
        m_previewItemId = item_id;
        m_previewCandidateId = 0;
        PositionPreviewWindow();
    } catch (...) {
        HidePreview();
    }
}

void MainWindow::ShowPreviewForCandidate() {
    if (m_popupVisible && m_previewCandidateId &&
        m_previewCandidateId == m_selectedItemId && !m_previewSuppressed) {
        ShowPreviewForItem(m_previewCandidateId);
    }
}

void MainWindow::ShowPreviewForSelection() {
    const int selected = SelectedHistoryIndex();
    if (selected < 0) {
        HidePreview();
        return;
    }
    m_keyboardHandler.SetActiveHistoryItem(selected, m_items, m_historyList, m_pinsList);
    m_selectedItemId = m_keyboardHandler.GetActiveItemId();
    ShowPreviewForItem(m_keyboardHandler.GetActiveItemId());
}

void MainWindow::HidePreview() {
    KillTimer(AppConstants::Timer::kPreview);
    m_previewCandidateId = 0;
    m_previewItemId = 0;
    m_previewWindow.Hide();
}

void MainWindow::TogglePreview() {
    if (m_previewWindow.IsVisible()) {
        m_previewSuppressed = true;
        HidePreview();
    } else {
        m_previewSuppressed = false;
        ShowPreviewForSelection();
    }
}

void MainWindow::PasteItem(int index) {
    if (m_loadingList || m_clipboardMonitor.IsPasting() ||
        index < 0 || static_cast<size_t>(index) >= m_items.size()) {
        return;
    }
    const HWND target = m_clipboardMonitor.GetTargetWindow();
    const HWND target_focus = m_clipboardMonitor.GetTargetFocusWindow();
    const sqlite3_int64 id = m_items[static_cast<size_t>(index)].id;
    try {
        const auto [paste, plain] = ResolvePasteAction(
            m_settings.paste_by_default,
            m_settings.remove_formatting_by_default,
            (GetKeyState(VK_CONTROL) & 0x8000) != 0,
            (GetKeyState(VK_MENU) & 0x8000) != 0,
            (GetKeyState(VK_SHIFT) & 0x8000) != 0
        );
        const auto item = m_database.GetItem(id, true);
        if (!item || !m_clipboardMonitor.SetClipboardItem(*item, plain)) {
            return;
        }
        m_clipboardMonitor.SetPasting(true);
        m_clipboardMonitor.SetSkipNextEvent(true);
        HideMainWindow();
        m_database.MarkCopied(id);
        m_clipboardMonitor.RestoreTargetFocusAndPaste(target, target_focus, paste);
        m_clipboardMonitor.SetPasting(false);
    } catch (const std::exception& error) {
        m_clipboardMonitor.SetPasting(false);
        OutputDebugStringA(error.what());
        OutputDebugStringA("\n");
    }
}

void MainWindow::PasteSelectedItem() {
    const int selected = SelectedHistoryIndex();
    if (selected >= 0) {
        PasteItem(selected);
    }
}

void MainWindow::ToggleSelectedPin() {
    const int index = SelectedHistoryIndex();
    if (index < 0) {
        return;
    }
    ClipboardItem& item = m_items[static_cast<size_t>(index)];
    try {
        if (item.pinned) {
            m_database.TogglePin(item.id, {}, false);
        } else {
            const std::wstring key = PinKeyPolicy::Next(
                m_database.SearchHistory({}, 0, 0, false),
                m_settings
            );
            if (key.empty()) {
                MessageBoxW(L"没有可用的置顶快捷键，请先取消一个置顶项目。", L"置顶", MB_OK);
                return;
            }
            m_database.TogglePin(item.id, key, true);
        }
        RequestUiUpdate(AppConstants::UiUpdate::kHistory);
    } catch (const std::exception& error) {
        OutputDebugStringA(error.what());
        OutputDebugStringA("\n");
    }
}

void MainWindow::DeleteSelectedItem() {
    const int index = SelectedHistoryIndex();
    if (index < 0) {
        return;
    }
    try {
        m_database.DeleteItem(m_items[static_cast<size_t>(index)].id);
        RequestUiUpdate(AppConstants::UiUpdate::kHistory);
    } catch (const std::exception& error) {
        OutputDebugStringA(error.what());
        OutputDebugStringA("\n");
    }
}

void MainWindow::ClearHistory(bool all) {
    const bool suppress = m_database.GetSetting(L"behavior.suppressClearAlert") == std::optional<std::wstring>(L"1");
    if (!suppress) {
        m_modalShowing = true;
        TASKDIALOGCONFIG config{sizeof(config)};
        config.hwndParent = m_hWnd;
        config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
        config.dwCommonButtons = TDCBF_YES_BUTTON | TDCBF_NO_BUTTON;
        config.nDefaultButton = IDNO;
        config.pszWindowTitle = L"清空历史";
        config.pszMainInstruction = all ? L"清空全部历史，包括置顶项目？" : L"清空未置顶的历史？";
        config.pszContent = m_settings.clear_system_clipboard ? L"此操作无法撤销，并将清空系统剪贴板。" : L"此操作无法撤销。";
        config.pszVerificationText = L"以后不再询问";
        int button = IDNO; BOOL checked = FALSE;
        const HRESULT result = TaskDialogIndirect(&config, &button, nullptr, &checked);
        if (FAILED(result)) {
            button = MessageBoxW(config.pszMainInstruction, config.pszWindowTitle,
                MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING);
        }
        m_modalShowing = false;
        if (button != IDYES) return;
        if (checked) m_database.SetSetting(L"behavior.suppressClearAlert", L"1");
    }
    try {
        if (all) {
            m_database.DeleteAll();
        } else {
            m_database.DeleteUnpinned();
        }
        if (m_settings.clear_system_clipboard && ::OpenClipboard(m_hWnd)) {
            EmptyClipboard();
            CloseClipboard();
        }
        ::SetWindowTextW(m_search, L"");
        RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout);
    } catch (const std::exception& error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to clear history", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::OpenAbout() {
    m_modalShowing = true;
    std::wstring about = L"maccy ";
    about += AppConstants::kAppVersion;
    about += L"\n\n轻量 Windows 剪贴板历史工具\n布局和交互参考 Maccy 2.7.1\n\n使用 C++、WTL 和 SQLite 构建。";
    const std::wstring caption = L"关于 maccy";
    MessageBoxW(about.c_str(), caption.c_str(), MB_OK | MB_ICONINFORMATION);
    m_modalShowing = false;
}

void MainWindow::OpenSettings() {
    HideMainWindow();
    if (m_settingsWindow == nullptr) {
        m_settingsWindow = std::make_unique<SettingsWindow>(m_database, m_hWnd);
    }
    if (!m_settingsWindow->CreateOrShow()) {
        ::MessageBoxW(m_hWnd, L"无法打开设置窗口。", L"maccy", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::ExitApplication() {
    m_exiting = true;
    if (m_settings.clear_on_quit) {
        try {
            m_database.DeleteUnpinned();
        } catch (...) {
        }
    }
    if (m_settings.clear_on_quit && m_settings.clear_system_clipboard && ::OpenClipboard(m_hWnd)) {
        EmptyClipboard();
        CloseClipboard();
    }
    RemoveTrayIcon();
    if (m_settingsWindow != nullptr) {
        m_settingsWindow->DestroyForOwner();
    }
    DestroyWindow();
}

void MainWindow::SaveWindowGeometry(bool resized) {
    RECT rect{};
    if (!::GetWindowRect(m_hWnd, &rect)) {
        return;
    }

    const int width = std::clamp(
        static_cast<int>(rect.right - rect.left),
        kMinimumPopupWidth,
        kMaximumPopupWidth
    );
    const int height = std::clamp(
        resized ? static_cast<int>(rect.bottom - rect.top) : m_settings.window_height,
        kMinimumPopupHeight,
        kMaximumPopupHeight
    );
    const bool persist_popup_position = m_activePopupPosition != PopupPosition::StatusItem;
    if (persist_popup_position) {
        m_settings.popup_x = rect.left;
        m_settings.popup_y = rect.top;
    }
    m_settings.window_width = width;
    m_settings.window_height = height;
    try {
        if (persist_popup_position) {
            m_database.SetSetting(L"appearance.windowX", std::to_wstring(m_settings.popup_x));
            m_database.SetSetting(L"appearance.windowY", std::to_wstring(m_settings.popup_y));
        }
        m_database.SetSetting(L"appearance.windowWidth", std::to_wstring(width));
        m_database.SetSetting(L"appearance.windowHeight", std::to_wstring(height));
    } catch (...) {
    }
}

void MainWindow::HideMainWindow() {
    if (!m_popupVisible) {
        return;
    }
    SaveWindowGeometry();
    m_popupVisible = false;
    HidePreview();
    ShowWindow(SW_HIDE);
}

int MainWindow::SelectedHistoryIndex() const {
    return m_keyboardHandler.GetActiveFooter() < 0 ? m_keyboardHandler.GetActiveItemIndex() : -1;
}

void MainWindow::ScheduleSearchFromCurrentEdit() {
    ScheduleSearch();
    RequestUiUpdate(AppConstants::UiUpdate::kLayout);
}

void MainWindow::UpdateTrayTooltip() {
    m_clipboardMonitor.UpdateTrayTooltip(m_notifyIcon, m_trayIconAdded);
}

void MainWindow::UpdateTrayIcon() {
    if (!m_settings.show_in_status_bar) {
        RemoveTrayIcon();
        return;
    }
    if (!m_trayIconAdded) {
        AddTrayIcon();
        return;
    }
    m_notifyIcon.hIcon = TrayIconForName(m_settings.menu_icon);
    m_notifyIcon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    UpdateTrayTooltip();
}

void MainWindow::ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    m_trayMenuShowing = true;
    AppendMenuW(menu, MF_STRING, kTrayCommandShow, L"打开剪贴板历史");
    AppendMenuW(menu, MF_STRING, kTrayCommandSettings, L"设置…");
    AppendMenuW(menu, MF_STRING, kTrayCommandClear, L"清除未置顶历史");
    AppendMenuW(
        menu,
        MF_STRING | (m_settings.ignore_events ? MF_CHECKED : MF_UNCHECKED),
        kTrayCommandIgnore,
        L"暂时忽略复制"
    );
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kTrayCommandExit, L"退出");
    POINT cursor{};
    GetCursorPos(&cursor);
    SetForegroundWindow(m_hWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_LEFTALIGN, cursor.x, cursor.y, 0, m_hWnd, nullptr);
    ::PostMessageW(m_hWnd, WM_NULL, 0, 0);
    m_trayMenuShowing = false;
    DestroyMenu(menu);
}

void MainWindow::RemoveTrayIcon() {
    if (m_trayIconAdded) {
        Shell_NotifyIconW(NIM_DELETE, &m_notifyIcon);
        m_trayIconAdded = false;
    }
}

void MainWindow::RequestUiUpdate(std::uint32_t updateMask) {
    if (updateMask == 0 || m_hWnd == nullptr || !::IsWindow(m_hWnd)) {
        return;
    }
    m_pendingUpdates |= updateMask;
    if (m_updateMessagePosted) {
        return;
    }
    m_updateMessagePosted = ::PostMessageW(
        m_hWnd, AppConstants::kUiUpdateMessage, 0, 0) != FALSE;
}

void MainWindow::ApplyPendingState() {
    std::uint32_t updates = m_pendingUpdates;
    m_pendingUpdates = 0;
    m_updateMessagePosted = false;
    if (updates == 0) {
        return;
    }

    if ((updates & AppConstants::UiUpdate::kIgnoreRules) != 0 &&
        (updates & AppConstants::UiUpdate::kSettings) == 0) {
        m_clipboardMonitor.ReloadIgnoreLists();
    }
    if ((updates & AppConstants::UiUpdate::kSettings) != 0) {
        updates |= ApplySettings(updates);
    }
    if ((updates & AppConstants::UiUpdate::kHistory) != 0) {
        RefreshHistory(m_search != nullptr ? ReadWindowText(m_search) : m_searchQuery);
    }
    // Rebuilding either history list can change the pinned/history section
    // counts, so its geometry must be recalculated together with the data.
    if ((updates & (AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout)) != 0) {
        ApplyHistoryVisibility();
    }
    if ((updates & AppConstants::UiUpdate::kTray) != 0) {
        UpdateTrayIcon();
    }
    if ((updates & AppConstants::UiUpdate::kFooter) != 0) {
        UpdateFooterControls();
        RedrawFooterButtons();
    }
}

std::uint32_t MainWindow::ApplySettings(std::uint32_t requestedUpdates) {
    const AppSettings previous = m_settings;
    m_settings = AppSettings::Load(m_database);

    std::uint32_t updates = 0;
    const bool ignoreRulesChanged =
        (requestedUpdates & AppConstants::UiUpdate::kIgnoreRules) != 0 ||
        previous.ignore_all_apps_except_listed != m_settings.ignore_all_apps_except_listed;
    if (ignoreRulesChanged) {
        m_clipboardMonitor.ReloadIgnoreLists();
    }

    const bool previewTipChanged = !SameHotKey(previous.preview_hotkey, m_settings.preview_hotkey);
    if (previewTipChanged && m_tooltips != nullptr) {
        m_previewTip = L"显示或隐藏预览（" + HotKeyToText(m_settings.preview_hotkey) + L"）";
        TOOLINFOW info{sizeof(info)};
        info.uFlags = TTF_IDISHWND;
        info.hwnd = m_hWnd;
        info.uId = reinterpret_cast<UINT_PTR>(m_previewToggle);
        info.lpszText = m_previewTip.data();
        SendMessageW(m_tooltips, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&info));
    }
    if (previous.preview_width != m_settings.preview_width) {
        m_previewWindow.SetWidth(m_settings.preview_width);
    }
    if (m_popupVisible &&
        (previous.window_width != m_settings.window_width ||
         previous.window_height != m_settings.window_height)) {
        RECT rect{};
        if (::GetWindowRect(m_hWnd, &rect)) {
            ::SetWindowPos(m_hWnd, HWND_TOPMOST, rect.left, rect.top,
                PopupWidth(), PopupHeight(), SWP_NOACTIVATE);
        }
    }
    if (!m_isolated && (!SameHotKey(previous.open_hotkey, m_settings.open_hotkey) ||
        previous.show_in_status_bar != m_settings.show_in_status_bar)) {
        m_keyboardHandler.UnregisterGlobalHotKey(AppConstants::HotKey::kOpenPopup);
        m_keyboardHandler.RegisterGlobalHotKey(AppConstants::HotKey::kOpenPopup);
    }

    const bool historyChanged =
        previous.search_mode != m_settings.search_mode ||
        previous.sort_by != m_settings.sort_by ||
        previous.pin_to != m_settings.pin_to ||
        previous.history_size != m_settings.history_size ||
        previous.show_special_symbols != m_settings.show_special_symbols;
    const bool layoutChanged =
        previous.show_search != m_settings.show_search ||
        previous.search_visibility != m_settings.search_visibility ||
        previous.show_title != m_settings.show_title ||
        previous.show_footer != m_settings.show_footer ||
        previous.pin_to != m_settings.pin_to ||
        previous.window_width != m_settings.window_width ||
        previous.window_height != m_settings.window_height ||
        previous.show_application_icons != m_settings.show_application_icons ||
        previous.show_hex_color_swatch != m_settings.show_hex_color_swatch ||
        previous.highlight_match != m_settings.highlight_match;
    if (historyChanged) {
        updates |= AppConstants::UiUpdate::kHistory;
    }
    if (layoutChanged) {
        updates |= AppConstants::UiUpdate::kLayout;
    }
    if (previewTipChanged || previous.menu_icon != m_settings.menu_icon ||
        previous.show_in_status_bar != m_settings.show_in_status_bar ||
        previous.show_recent_copy_in_menu_bar != m_settings.show_recent_copy_in_menu_bar) {
        updates |= AppConstants::UiUpdate::kTray;
    }
    if (!m_settings.show_search) {
        const bool searchHadText = !m_searchQuery.empty() || !ReadWindowText(m_search).empty();
        if (searchHadText) {
            ::SetWindowTextW(m_search, L"");
            m_searchQuery.clear();
            updates |= AppConstants::UiUpdate::kHistory;
        }
        updates |= AppConstants::UiUpdate::kLayout;
    }
    return updates;
}

// Callback implementations for KeyboardHandler
void MainWindow::OnPreviewCallback(void* context, sqlite3_int64 itemId, bool) {
    auto* window = static_cast<MainWindow*>(context);
    window->m_selectedItemId = itemId;
    window->SchedulePreviewForItem(itemId);
}

void MainWindow::OnPasteCallback(void* context, int index) {
    auto* window = static_cast<MainWindow*>(context);
    window->PasteItem(index);
}

void MainWindow::OnScheduleSearchCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    const auto query = ReadWindowText(window->m_search);
    if (query != window->m_searchQuery) {
        ::KillTimer(window->m_hWnd, AppConstants::Timer::kSearch);
        window->RequestUiUpdate(AppConstants::UiUpdate::kHistory);
    }
}

void MainWindow::OnTogglePinCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    window->ToggleSelectedPin();
}

void MainWindow::OnDeleteItemCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    window->DeleteSelectedItem();
}

void MainWindow::OnTogglePreviewCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    window->TogglePreview();
}

void MainWindow::OnClearHistoryCallback(void* context, bool all) {
    auto* window = static_cast<MainWindow*>(context);
    window->ClearHistory(all);
}

void MainWindow::OnOpenSettingsCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    window->OpenSettings();
}

void MainWindow::OnExitCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    window->ExitApplication();
}

void MainWindow::OnHideWindowCallback(void* context) {
    auto* window = static_cast<MainWindow*>(context);
    window->HideMainWindow();
}

// Message handlers
LRESULT MainWindow::OnInitDialog(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    if (!m_historyRenderer.Initialize(m_hWnd) ||
        !BindControls() ||
        !m_previewWindow.Initialize(m_hWnd, m_settings.preview_width)) {
        handled = FALSE;
        return FALSE;
    }

    m_keyboardHandler.Initialize(m_hWnd, m_search, m_historyList, m_pinsList, FooterButtons());
    m_keyboardHandler.SetCallbacks(
        this,
        OnPreviewCallback,
        OnPasteCallback,
        OnScheduleSearchCallback,
        OnTogglePinCallback,
        OnDeleteItemCallback,
        OnTogglePreviewCallback,
        OnClearHistoryCallback,
        OnOpenSettingsCallback,
        OnExitCallback,
        OnHideWindowCallback
    );

    m_clipboardMonitor.ReloadIgnoreLists();
    if (!m_isolated) {
        m_clipboardMonitor.Initialize(m_hWnd);
        m_keyboardHandler.RegisterGlobalHotKey(AppConstants::HotKey::kOpenPopup);
    }
    RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout |
                    AppConstants::UiUpdate::kTray);
    return TRUE;
}

LRESULT MainWindow::OnSize(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    LayoutHistoryControls();
    if (m_previewWindow.IsVisible()) {
        PositionPreviewWindow();
    }
    return 0;
}

LRESULT MainWindow::OnDpiChanged(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled) {
    handled = TRUE;
    if (m_historyRenderer.UpdateFonts(HIWORD(wParam))) {
        ApplyHistoryFonts();
    }
    const auto* suggested = reinterpret_cast<const RECT*>(lParam);
    if (suggested != nullptr) {
        ::SetWindowPos(m_hWnd, nullptr, suggested->left, suggested->top,
            suggested->right - suggested->left, suggested->bottom - suggested->top,
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
    LayoutHistoryControls();
    RedrawHistoryLists();
    RedrawFooterButtons();
    return 0;
}

LRESULT MainWindow::OnMove(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    if (m_previewWindow.IsVisible()) {
        PositionPreviewWindow();
    }
    return 0;
}

LRESULT MainWindow::OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    m_inSizeMove = false;
    SaveWindowGeometry(true);
    if (m_previewWindow.IsVisible()) {
        PositionPreviewWindow();
    }
    return 0;
}

LRESULT MainWindow::OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL& handled) {
    m_inSizeMove = true;
    handled = TRUE;
    return 0;
}

LRESULT MainWindow::OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
    if (info == nullptr) {
        handled = FALSE;
        return 0;
    }
    info->ptMinTrackSize.x = kMinimumPopupWidth;
    info->ptMinTrackSize.y = kMinimumPopupHeight;
    info->ptMaxTrackSize.x = kMaximumPopupWidth;
    info->ptMaxTrackSize.y = kMaximumPopupHeight;
    handled = TRUE;
    return 0;
}

LRESULT MainWindow::OnNcHitTest(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    RECT rect{};
    if (!::GetWindowRect(m_hWnd, &rect)) {
        handled = FALSE;
        return 0;
    }

    const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    const bool left = point.x < rect.left + kResizeBorder;
    const bool right = point.x >= rect.right - kResizeBorder;
    const bool top = point.y < rect.top + kResizeBorder;
    const bool bottom = point.y >= rect.bottom - kResizeBorder;

    handled = TRUE;
    if (top && left) return HTTOPLEFT;
    if (top && right) return HTTOPRIGHT;
    if (bottom && left) return HTBOTTOMLEFT;
    if (bottom && right) return HTBOTTOMRIGHT;
    if (left) return HTLEFT;
    if (right) return HTRIGHT;
    if (top) return HTTOP;
    if (bottom) return HTBOTTOM;
    return HTCAPTION;
}

LRESULT MainWindow::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(&paint);
    RECT client{};
    GetClientRect(&client);
    m_historyRenderer.OnPaint(dc, client, m_pinSeparatorY, m_footerSeparatorY,
        ::IsWindowVisible(m_search) != FALSE, m_searchRect, m_titleRect);
    EndPaint(&paint);
    return 0;
}

LRESULT MainWindow::OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&) {
    return 1;
}

LRESULT MainWindow::OnEditColor(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled) {
    const bool title = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == IDC_HISTORY_TITLE;
    handled = reinterpret_cast<HWND>(lParam) == m_search || title;
    if (!handled) return 0;
    HDC dc = reinterpret_cast<HDC>(wParam);
    SetTextColor(dc, GetSysColor(title ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
    SetBkColor(dc, GetSysColor(title ? COLOR_WINDOW : COLOR_BTNFACE));
    return reinterpret_cast<LRESULT>(GetSysColorBrush(title ? COLOR_WINDOW : COLOR_BTNFACE));
}

LRESULT MainWindow::OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    auto* measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
    if (measure == nullptr || (measure->CtlID != kHistoryListControlId && measure->CtlID != IDC_HISTORY_PINS)) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    measure->itemHeight = kHistoryItemHeight;
    return 0;
}

LRESULT MainWindow::OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
    if (draw && draw->CtlType == ODT_BUTTON) {
        m_historyRenderer.DrawMenuButton(draw, m_keyboardHandler.GetActiveFooter(), FooterButtons());
        handled = TRUE;
        return 0;
    }
    if (draw == nullptr || (draw->CtlID != kHistoryListControlId && draw->CtlID != IDC_HISTORY_PINS)) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    m_historyRenderer.DrawHistoryItem(draw, m_items, m_keyboardHandler.GetActiveItemIndex(),
        m_keyboardHandler.GetActiveFooter(), m_searchQuery);
    return 0;
}

LRESULT MainWindow::OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
    if (LOWORD(wParam) == WA_INACTIVE && !m_modalShowing) {
        const HWND activating_window = reinterpret_cast<HWND>(lParam);
        if (activating_window != nullptr && !m_trayMenuShowing && !IsOurWindow(activating_window)) {
            m_clipboardMonitor.CaptureTargetWindow(activating_window);
        }
        if (m_popupVisible && !m_exiting && !m_trayMenuShowing && !IsOurWindow(activating_window)) {
            HideMainWindow();
        }
    }
    return 0;
}

LRESULT MainWindow::OnClose(UINT, WPARAM, LPARAM, BOOL&) {
    if (m_exiting) {
        DestroyWindow();
    } else {
        HideMainWindow();
    }
    return 0;
}

LRESULT MainWindow::OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled) {
    const UINT command = LOWORD(wParam);
    const UINT notification = HIWORD(wParam);

    if (notification == BN_CLICKED && command >= IDC_PREVIEW_PIN && command <= IDC_PREVIEW_CLOSE) {
        handled = TRUE;
        if (command == IDC_PREVIEW_CLOSE) {
            m_previewSuppressed = true;
            HidePreview();
            m_keyboardHandler.FocusSearchOrPopup(m_search, m_hWnd, ::IsWindowVisible(m_search) != FALSE);
        } else {
            m_keyboardHandler.SetActiveHistoryItem(m_keyboardHandler.GetActiveItemIndex(),
                m_items, m_historyList, m_pinsList);
            if (command == IDC_PREVIEW_PIN) ToggleSelectedPin();
            else DeleteSelectedItem();
        }
        return 0;
    }

    if (command == kTrayCommandShow && notification == 0) {
        handled = TRUE;
        ShowMainWindow(PopupPosition::StatusItem);
        return 0;
    }
    if (command == kTrayCommandSettings && notification == 0) {
        handled = TRUE;
        OpenSettings();
        return 0;
    }
    if (command == kTrayCommandClear && notification == 0) {
        handled = TRUE;
        ClearHistory();
        return 0;
    }
    if (command == kTrayCommandIgnore && notification == 0) {
        handled = TRUE;
        m_settings.ignore_events = !m_settings.ignore_events;
        if (!m_settings.ignore_events) {
            m_settings.ignore_only_next_event = false;
        }
        try {
            m_settings.Save(m_database);
        } catch (...) {
        }
        RequestUiUpdate(AppConstants::UiUpdate::kFooter);
        return 0;
    }
    if (command == kTrayCommandExit && notification == 0) {
        handled = TRUE;
        ExitApplication();
        return 0;
    }
    if (notification == BN_CLICKED && command == IDC_HISTORY_CLEAR) {
        handled = TRUE;
        ClearHistory((GetKeyState(VK_SHIFT) & 0x8000) != 0);
        return 0;
    }
    if (notification == BN_CLICKED && command == IDC_HISTORY_SETTINGS) {
        handled = TRUE;
        OpenSettings();
        return 0;
    }
    if (notification == BN_CLICKED && command == IDC_HISTORY_ABOUT) {
        handled = TRUE;
        OpenAbout();
        return 0;
    }
    if (notification == BN_CLICKED && command == IDC_HISTORY_SEARCH_CLEAR) {
        handled = TRUE;
        ::SetWindowTextW(m_search, L"");
        RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout);
        m_keyboardHandler.FocusSearchOrPopup(m_search, m_hWnd, ::IsWindowVisible(m_search) != FALSE);
        return 0;
    }
    if (notification == BN_CLICKED && command == IDC_HISTORY_PREVIEW) {
        handled = TRUE;
        TogglePreview();
        m_keyboardHandler.FocusSearchOrPopup(m_search, m_hWnd, ::IsWindowVisible(m_search) != FALSE);
        return 0;
    }
    if (notification == BN_CLICKED && command == IDC_HISTORY_EXIT) {
        handled = TRUE;
        ExitApplication();
        return 0;
    }
    if (command == kSearchControlId && notification == EN_CHANGE) {
        handled = TRUE;
        ScheduleSearchFromCurrentEdit();
        return 0;
    }
    if ((command == kHistoryListControlId || command == IDC_HISTORY_PINS) && notification == LBN_SELCHANGE) {
        handled = TRUE;
        if (!m_loadingList) {
            const HWND list = reinterpret_cast<HWND>(lParam);
            const LRESULT selected = SendMessageW(list, LB_GETCURSEL, 0, 0);
            if (selected != LB_ERR) {
                const int selected_index = m_keyboardHandler.ItemIndexAtRow(
                    list,
                    static_cast<int>(selected),
                    m_items
                );
                if (selected_index >= 0) {
                    const bool selected_by_mouse = m_keyboardHandler.GetHoveredItemIndex() == selected_index;
                    m_keyboardHandler.SetActiveHistoryItem(selected_index, m_items, m_historyList,
                        m_pinsList, !selected_by_mouse);
                    m_selectedItemId = m_keyboardHandler.GetActiveItemId();
                    if (!selected_by_mouse) {
                        m_keyboardHandler.ClearHistoryHover();
                        if (m_previewWindow.IsVisible()) {
                            ShowPreviewForItem(m_keyboardHandler.GetActiveItemId());
                        } else {
                            SchedulePreviewForItem(m_keyboardHandler.GetActiveItemId());
                        }
                    }
                }
            }
        }
        return 0;
    }
    handled = FALSE;
    return 0;
}

LRESULT MainWindow::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
    if (wParam == AppConstants::Timer::kPaste) {
        handled = TRUE;
        m_clipboardMonitor.OnPasteTimer(m_hWnd);
        return 0;
    }
    if (wParam == AppConstants::Timer::kSearch) {
        handled = TRUE;
        KillTimer(AppConstants::Timer::kSearch);
        RequestUiUpdate(AppConstants::UiUpdate::kHistory);
        return 0;
    }
    if (wParam == AppConstants::Timer::kPreview) {
        handled = TRUE;
        KillTimer(AppConstants::Timer::kPreview);
        ShowPreviewForCandidate();
        return 0;
    }
    handled = FALSE;
    return 0;
}

LRESULT MainWindow::OnClipboardUpdate(UINT, WPARAM, LPARAM, BOOL&) {
    if (m_clipboardMonitor.OnClipboardUpdate()) {
        RequestUiUpdate(AppConstants::UiUpdate::kTray);
        if (m_popupVisible) {
            RequestUiUpdate(AppConstants::UiUpdate::kHistory);
        }
    }
    return 0;
}

LRESULT MainWindow::OnHotKey(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
    if (wParam != AppConstants::HotKey::kOpenPopup) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    if (m_popupVisible) {
        HideMainWindow();
    } else {
        ShowMainWindow();
    }
    return 0;
}

LRESULT MainWindow::OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
    RequestFooterUpdateForKeyMessage(WM_KEYDOWN, wParam);
    handled = !m_keyboardHandler.IsComposing(m_hWnd) &&
              m_keyboardHandler.HandlePopupKey(wParam, m_search, m_items);
    return 0;
}

LRESULT MainWindow::OnKeyUp(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
    RequestFooterUpdateForKeyMessage(WM_KEYUP, wParam);
    handled = FALSE;
    return 0;
}

LRESULT MainWindow::OnChar(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
    handled = m_keyboardHandler.OnChar(wParam);
    if (handled) m_keyboardHandler.TypeToSearch(wParam, m_search);
    return 0;
}

LRESULT MainWindow::OnImeStart(UINT, WPARAM, LPARAM, BOOL& handled) {
    m_keyboardHandler.SetImeComposing(true);
    handled = FALSE;
    return 0;
}

LRESULT MainWindow::OnImeEnd(UINT, WPARAM, LPARAM, BOOL& handled) {
    m_keyboardHandler.SetImeComposing(false);
    handled = FALSE;
    return 0;
}

LRESULT MainWindow::OnTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
    if (wParam != kTrayIconId) {
        return 0;
    }
    switch (static_cast<UINT>(lParam)) {
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
        if (GetKeyState(VK_MENU) & 0x8000) {
            m_settings.ignore_events = !m_settings.ignore_events;
            m_settings.ignore_only_next_event = false;
            m_settings.Save(m_database);
        } else {
            ShowMainWindow(PopupPosition::StatusItem);
        }
        break;
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        m_clipboardMonitor.CaptureTargetWindow();
        ShowTrayMenu();
        break;
    default:
        break;
    }
    return 0;
}

LRESULT MainWindow::OnUiUpdate(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
    handled = TRUE;
    m_pendingUpdates |= static_cast<std::uint32_t>(wParam);
    ApplyPendingState();
    return 0;
}

LRESULT MainWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
    m_popupVisible = false;
    KillTimer(AppConstants::Timer::kSearch);
    KillTimer(AppConstants::Timer::kPreview);
    m_keyboardHandler.Shutdown();
    m_clipboardMonitor.Shutdown();
    if (m_previewWindow.Window() != nullptr && ::IsWindow(m_previewWindow.Window())) {
        m_previewWindow.DestroyWindow();
    }
    RestoreControlSubclasses();
    RemoveTrayIcon();
    m_historyRenderer.Shutdown();
    PostQuitMessage(0);
    return 0;
}

bool MainWindow::AddTrayIcon() {
    if (!m_settings.show_in_status_bar) {
        RemoveTrayIcon();
        return true;
    }
    m_notifyIcon = {};
    m_notifyIcon.cbSize = sizeof(m_notifyIcon);
    m_notifyIcon.hWnd = m_hWnd;
    m_notifyIcon.uID = kTrayIconId;
    m_notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_notifyIcon.uCallbackMessage = AppConstants::kTrayIconMessage;
    m_notifyIcon.hIcon = TrayIconForName(m_settings.menu_icon);
    UpdateTrayTooltip();
    m_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &m_notifyIcon) == TRUE;
    return m_trayIconAdded;
}

void MainWindow::ShowMainWindow() {
    ShowMainWindow(m_settings.popup_position);
}

void MainWindow::ShowMainWindow(PopupPosition popup_position) {
    m_activePopupPosition = popup_position;
    m_keyboardHandler.ClearHistoryHover();
    m_clipboardMonitor.CaptureTargetWindow();
    m_previewSuppressed = false;
    ::SetWindowTextW(m_search, L"");
    KillTimer(AppConstants::Timer::kSearch);
    HidePreview();
    RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout);
    PositionPopup(popup_position);

    m_popupVisible = true;
    ShowWindow(SW_SHOW);
    ApplyPendingState();
    SetForegroundWindow(m_hWnd);
    if (m_settings.show_search && m_settings.search_visibility == SearchVisibility::Always) {
        SetHistorySearchVisible(true);
        ::SetFocus(m_search);
        SendMessageW(m_search, EM_SETSEL, 0, -1);
    } else {
        ::SetFocus(m_hWnd);
    }
    UpdateWindow();
    m_keyboardHandler.UpdateHistoryHoverFromCursor(m_items, m_historyList, m_pinsList, m_popupVisible);
}
