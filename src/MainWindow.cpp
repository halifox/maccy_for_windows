#include "MainWindow.h"
#include "ClipboardMonitor.h"
#include "Constants.h"
#include "PinKeys.h"
#include "SettingsWindow.h"
#include "TrayIcon.h"
#include "UiFont.h"

#include <shellapi.h>

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

constexpr int kSearchControlId = IDC_HISTORY_SEARCH;
constexpr int kHistoryListControlId = IDC_HISTORY_LIST;

constexpr int kHistoryItemHeight = 22;
constexpr int kHistoryFooterHeight = 22;
constexpr int kHistoryFooterGap = 6;
constexpr int kHistorySectionGap = 6;
constexpr int kResizeBorder = 8;

constexpr wchar_t kHistorySearchCue[] = L"搜索剪贴板内容…";
constexpr wchar_t kControlOwnerProperty[] = L"maccyMainWindow";

std::wstring DisplayVersion(std::wstring_view version) {
    if (!version.empty() && (version.front() == L'v' || version.front() == L'V')) {
        return std::wstring(version);
    }
    return L"v" + std::wstring(version);
}

bool OpenReleasePage(HWND owner, std::wstring_view url) {
    const std::wstring target(url);
    const HINSTANCE result = ::ShellExecuteW(
        owner,
        L"open",
        target.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );
    return reinterpret_cast<INT_PTR>(result) > 32;
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

bool IsShiftKey(WPARAM key) {
    return key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT;
}

bool HasPendingKeyboardInput(HWND window) {
    if (window == nullptr || !::IsWindow(window)) {
        return false;
    }
    MSG message{};
    return ::PeekMessageW(&message, window, WM_KEYFIRST, WM_KEYLAST, PM_NOREMOVE) != FALSE;
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

MainWindow::MainWindow(
    StorageWorker &storage,
    PreviewWorker &preview,
    AppSettings settings,
    StorageWorker::IgnoreLists ignored_lists,
    bool isolated
)
    : m_storage(storage),
      m_settings(std::move(settings)),
      m_suppressClearAlert(storage.LoadSuppressClearAlert()),
      m_ignoredLists(std::move(ignored_lists)),
      m_clipboard(m_settings, m_ignoredLists),
      m_previewWorker(preview),
      m_historyRenderer(m_settings),
      m_keyboardHandler(m_settings),
      m_isolated(isolated) {
    m_clipboard.SetSaveCallback([this](ClipboardSnapshot capture) {
        m_storage.SaveClipboardAsync(
            std::move(capture),
            m_settings.history_size,
            [this](bool success, std::string error) {
                if (!success) {
                    ::OutputDebugStringA(error.c_str());
                    ::OutputDebugStringA("\n");
                    return;
                }
                RequestUiUpdate(
                    AppConstants::UiUpdate::kTray |
                    (m_popupVisible ? AppConstants::UiUpdate::kHistory : 0)
                );
            }
        );
    });
}

MainWindow::~MainWindow() {
    m_updateChecker.Stop();
    m_clipboard.SetSaveCallback({});
    m_previewWorker.SetUiWindow(nullptr);
    m_storage.SetUiWindow(nullptr);
}

void MainWindow::DrawSearchCue(HWND window, HDC dc) const {
    if (window == nullptr || dc == nullptr || window != m_search ||
        ::GetFocus() == window || !ReadWindowText(window).empty()) {
        return;
    }

    RECT rect{};
    ::GetClientRect(window, &rect);
    const DWORD margins = static_cast<DWORD>(::SendMessageW(window, EM_GETMARGINS, 0, 0));
    rect.left += LOWORD(margins);
    rect.right -= HIWORD(margins);
    if (rect.right <= rect.left || rect.bottom <= rect.top) {
        return;
    }

    const HFONT font = m_historyRenderer.GetNormalFont();
    const HGDIOBJ previous_font = font != nullptr ? ::SelectObject(dc, font) : nullptr;
    const int previous_mode = ::SetBkMode(dc, TRANSPARENT);
    const COLORREF previous_color = ::SetTextColor(dc, ::GetSysColor(COLOR_GRAYTEXT));
    ::DrawTextW(dc, kHistorySearchCue, -1, &rect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    ::SetTextColor(dc, previous_color);
    ::SetBkMode(dc, previous_mode);
    if (previous_font != nullptr) {
        ::SelectObject(dc, previous_font);
    }
}

bool MainWindow::IsSearchClearHit(POINT point) const {
    return m_search != nullptr && ::IsWindowVisible(m_search) != FALSE &&
        m_searchHeader.showSearch && !ReadWindowText(m_search).empty() &&
        ::PtInRect(&m_searchHeader.searchClear, point) != FALSE;
}

void MainWindow::ClearSearch() {
    if (m_search == nullptr) {
        return;
    }
    ::SetWindowTextW(m_search, L"");
    RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout);
    m_keyboardHandler.FocusSearchOrPopup(m_search, m_hWnd,
        ::IsWindowVisible(m_search) != FALSE);
}

LRESULT CALLBACK MainWindow::SearchWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* owner = reinterpret_cast<MainWindow*>(GetPropW(window, kControlOwnerProperty));
    if (!owner) return ::DefWindowProcW(window, message, wParam, lParam);
    owner->RequestFooterUpdateForKeyMessage(message, wParam);
    if (message == WM_IME_STARTCOMPOSITION) owner->m_keyboardHandler.SetImeComposing(true);
    if (message == WM_IME_ENDCOMPOSITION) owner->m_keyboardHandler.SetImeComposing(false);
    if (message == WM_KEYDOWN && wParam == 'U' &&
        !owner->m_keyboardHandler.IsComposing(window) &&
        (::GetKeyState(VK_CONTROL) & 0x8000) != 0) {
        if (!ReadWindowText(window).empty()) {
            owner->ClearSearch();
        }
        return 0;
    }
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
        !owner->m_keyboardHandler.IsComposing(window) &&
        owner->m_keyboardHandler.HandlePopupKey(wParam, owner->m_search, owner->m_items)) {
        return 0;
    }

    // Search from EN_CHANGE after the edit control has actually changed its
    // text. Scheduling from WM_KEYDOWN can enqueue a query for the old text.
    const LRESULT result = CallWindowProcW(owner->m_originalSearchProc, window, message, wParam, lParam);
    if (message == WM_SETFOCUS || message == WM_KILLFOCUS) {
        ::InvalidateRect(window, nullptr, TRUE);
    }
    if (message == WM_PAINT && owner->m_search == window && ReadWindowText(window).empty()) {
        HDC dc = ::GetDC(window);
        if (dc != nullptr) {
            owner->DrawSearchCue(window, dc);
            ::ReleaseDC(window, dc);
        }
    }
    return result;
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
        ::GetDlgCtrlID(window) == IDC_HISTORY_PREVIEW) {
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

std::array<HWND, AppConstants::UI::kFooterButtonCount> MainWindow::FooterButtons() const {
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
    m_footerClear = ::GetDlgItem(m_hWnd, IDC_HISTORY_CLEAR);
    m_footerSettings = ::GetDlgItem(m_hWnd, IDC_HISTORY_SETTINGS);
    m_footerAbout = ::GetDlgItem(m_hWnd, IDC_HISTORY_ABOUT);
    m_footerExit = ::GetDlgItem(m_hWnd, IDC_HISTORY_EXIT);

    if (m_search == nullptr || m_historyList == nullptr ||
        m_footerClear == nullptr || m_footerSettings == nullptr ||
        m_footerAbout == nullptr || m_footerExit == nullptr) {
        return false;
    }

    // The preview toggle is created here so SearchHeaderLayout owns its runtime
    // geometry instead of relying on a placeholder RC position.
    const DWORD buttonStyle = WS_CHILD | WS_TABSTOP | BS_OWNERDRAW;
    m_previewToggle = ::CreateWindowExW(0, L"BUTTON", L"预览", buttonStyle,
        0, 0, 1, 1, m_hWnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HISTORY_PREVIEW)),
        _Module.GetModuleInstance(), nullptr);
    if (m_previewToggle == nullptr) {
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
    SetWindowSubclass(m_previewToggle, MenuControlProc, 1, reinterpret_cast<DWORD_PTR>(this));

    // Remove borders
    for (HWND control : {m_search, m_historyList, m_pinsList}) {
        ::SetWindowLongPtrW(control, GWL_STYLE, ::GetWindowLongPtrW(control, GWL_STYLE) & ~WS_BORDER);
        ::SetWindowLongPtrW(control, GWL_EXSTYLE, ::GetWindowLongPtrW(control, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
        ::SetWindowPos(control, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }

    ::SetWindowTextW(m_previewToggle, L"预览");
    m_previewTip = L"显示或隐藏预览（" + HotKeyToText(m_settings.preview_hotkey) + L"）";

    m_tooltips = ::CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr, WS_POPUP | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr,
        _Module.GetModuleInstance(), nullptr);
    TOOLINFOW previewInfo{sizeof(previewInfo)};
    previewInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    previewInfo.hwnd = m_hWnd;
    previewInfo.uId = reinterpret_cast<UINT_PTR>(m_previewToggle);
    previewInfo.lpszText = m_previewTip.data();
    SendMessageW(m_tooltips, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&previewInfo));

    LONG_PTR search_style = ::GetWindowLongPtrW(m_search, GWL_STYLE);
    search_style &= ~static_cast<LONG_PTR>(ES_MULTILINE);
    search_style |= ES_AUTOHSCROLL;
    // The empty-state cue is painted by SearchWindowProc so it shares the edit's
    // actual client rectangle and vertical-centering rules.
    ::SetWindowLongPtrW(m_search, GWL_STYLE, search_style);

    return true;
}

void MainWindow::ApplyHistoryFonts() {
    const HFONT font = m_historyRenderer.GetNormalFont();
    for (HWND control : {m_search, m_historyList, m_pinsList, m_footerClear, m_footerSettings,
                         m_footerAbout, m_footerExit, m_previewToggle}) {
        if (control != nullptr) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }
}

void MainWindow::LayoutHistoryControls() {
    if (!m_search || !m_historyList || !::IsWindow(m_hWnd)) return;
    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    const SearchHeaderLayout::Metrics headerMetrics =
        SearchHeaderLayout::ForDpi(UiFont::DpiForWindow(m_hWnd));
    const int margin = headerMetrics.windowMargin;
    const int width = std::max(1L, client.right - 2 * margin);
    const bool header = ::IsWindowVisible(m_search) != FALSE;
    int titleWidth = 0;

    if (header && m_settings.show_title) {
        HDC dc = ::GetDC(m_hWnd);
        if (dc != nullptr) {
            const HFONT font = m_historyRenderer.GetSmallFont();
            const HGDIOBJ oldFont = font != nullptr ? ::SelectObject(dc, font) : nullptr;
            SIZE textSize{};
            if (::GetTextExtentPoint32W(dc, L"maccy", 5, &textSize)) {
                titleWidth = textSize.cx + headerMetrics.titlePadding;
            }
            if (oldFont != nullptr) ::SelectObject(dc, oldFont);
            ::ReleaseDC(m_hWnd, dc);
        }
    }

    const SearchHeaderLayout::Geometry headerLayout =
        SearchHeaderLayout::Calculate(client, titleWidth, header, headerMetrics);

    const int top = margin + (header ? headerMetrics.height + headerMetrics.contentGap : 0);
    const int footerHeight = m_settings.show_footer
        ? kHistoryFooterGap + kHistoryFooterHeight * AppConstants::UI::kFooterButtonCount
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

    m_searchHeader = headerLayout;

    if (header) {
        const auto place = [](HWND control, const RECT& rect) {
            ::SetWindowPos(control, nullptr, rect.left, rect.top,
                rect.right - rect.left, rect.bottom - rect.top,
                SWP_NOZORDER | SWP_NOACTIVATE);
        };
        place(m_search, headerLayout.searchEdit);
        place(m_previewToggle, headerLayout.preview);
    } else {
        for (HWND control : {m_search, m_previewToggle}) {
            ::SetWindowPos(control, nullptr, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
        }
    }

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
        ::IsChild(m_hWnd, window) || m_previewWorker.ContainsWindow(window) ||
        (m_settingsWindow != nullptr && window == m_settingsWindow->Window());
}

void MainWindow::HandlePopupActivation(HWND activating_window) {
    if (m_modalShowing) {
        return;
    }

    const bool outside_popup = !IsOurWindow(activating_window);
    if (activating_window != nullptr && !m_trayMenuShowing && outside_popup) {
        m_pasteController.CaptureTargetWindow(activating_window);
    }
    if (m_popupVisible && !m_exiting && !m_trayMenuShowing && outside_popup) {
        HideMainWindow();
    }
}

int MainWindow::PopupWidth() const {
    return std::clamp(
        m_settings.window_width,
        AppConstants::UI::kMinimumPopupWidth,
        AppConstants::UI::kMaximumPopupWidth
    );
}

int MainWindow::PopupHeight() const {
    return std::clamp(
        m_settings.window_height,
        AppConstants::UI::kMinimumPopupHeight,
        AppConstants::UI::kMaximumPopupHeight
    );
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
    const bool has_target = m_pasteController.GetTargetWindow() != nullptr
        && ::IsWindow(m_pasteController.GetTargetWindow())
        && ::GetWindowRect(m_pasteController.GetTargetWindow(), &target) == TRUE;
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

void MainWindow::RefreshHistory(std::wstring_view query) {
    const std::wstring owned_query(query);
    const int search_mode = static_cast<int>(m_settings.search_mode);
    const int sort_by = m_settings.sort_by;
    const bool pins_at_bottom = m_settings.pin_to == PinPosition::Bottom;
    const std::uint64_t generation = ++m_historyGeneration;
    m_loadingList = true;
    m_storage.SearchHistoryAsync(
        owned_query,
        search_mode,
        sort_by,
        pins_at_bottom,
        generation,
        [this, owned_query](std::uint64_t result_generation,
                            std::vector<ClipboardItem> items,
                            std::string error) mutable {
            if (result_generation != m_historyGeneration ||
                m_hWnd == nullptr || !::IsWindow(m_hWnd) || m_search == nullptr ||
                owned_query != ReadWindowText(m_search)) {
                return;
            }
            if (!error.empty()) {
                m_loadingList = false;
                ::OutputDebugStringA(error.c_str());
                ::OutputDebugStringA("\n");
                return;
            }
            if (HasPendingKeyboardInput(m_search)) {
                m_deferredHistoryResult = DeferredHistoryResult{
                    result_generation,
                    std::move(owned_query),
                    std::move(items)
                };
                if (::SetTimer(m_hWnd, AppConstants::Timer::kSearchResultCommit,
                               USER_TIMER_MINIMUM, nullptr) != 0) {
                    return;
                }
                ApplyDeferredHistoryResult();
                return;
            }
            KillTimer(AppConstants::Timer::kSearchResultCommit);
            m_deferredHistoryResult.reset();
            ApplyHistoryItems(std::move(owned_query), std::move(items));
        }
    );
}

void MainWindow::ApplyDeferredHistoryResult() {
    KillTimer(AppConstants::Timer::kSearchResultCommit);
    if (!m_deferredHistoryResult.has_value()) {
        return;
    }

    auto result = std::move(*m_deferredHistoryResult);
    m_deferredHistoryResult.reset();
    if (result.generation != m_historyGeneration ||
        m_hWnd == nullptr || !::IsWindow(m_hWnd) || m_search == nullptr ||
        result.query != ReadWindowText(m_search)) {
        return;
    }
    ApplyHistoryItems(std::move(result.query), std::move(result.items));
}

void MainWindow::ApplyHistoryItems(
    std::wstring query,
    std::vector<ClipboardItem> items
) {
    if (m_hWnd == nullptr || !::IsWindow(m_hWnd)) {
        return;
    }
    try {
        const bool sameQuery = query == m_searchQuery;
        const sqlite3_int64 previous = sameQuery ? m_keyboardHandler.GetActiveItemId() : 0;
        const int previousIndex = m_keyboardHandler.GetActiveItemIndex();
        const bool previewOpen = m_previewWorker.IsVisible();
        const std::wstring ownedQuery = query;
        m_items = std::move(items);
        m_historyRenderer.PrepareHistory(m_items);

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
        ApplyHistoryVisibility();
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

void MainWindow::SchedulePreviewForItem(sqlite3_int64 item_id) {
    const bool preview_requested = m_previewWorker.IsVisible() || m_previewCandidateId != 0;
    KillTimer(AppConstants::Timer::kPreview);
    if (!m_popupVisible || m_previewSuppressed || item_id == 0) {
        m_previewCandidateId = 0;
        return;
    }
    if (preview_requested) {
        if (m_previewItemId != item_id || m_previewCandidateId != item_id) {
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
    const std::uint64_t generation = ++m_previewGeneration;
    const bool preview_was_visible = m_previewWorker.IsVisible();
    m_previewCandidateId = item_id;
    if (!m_previewWorker.Request(
            item_id,
            generation,
            [this](std::shared_ptr<PreviewResult> result) {
                if (result == nullptr || result->generation != m_previewGeneration ||
                    result->item_id != m_keyboardHandler.GetActiveItemId() ||
                    !m_popupVisible || m_previewSuppressed) {
                    return;
                }
                if (!result->error.empty()) {
                    ::MessageBoxA(m_hWnd, result->error.c_str(), "无法加载预览", MB_OK | MB_ICONERROR);
                    HidePreview();
                    return;
                }
                if (!result->displayed) {
                    HidePreview();
                    return;
                }
                m_previewItemId = result->item_id;
                m_previewCandidateId = 0;
            }
        )) {
        m_previewCandidateId = 0;
        if (!preview_was_visible) {
            HidePreview();
        }
    }
}

void MainWindow::ShowPreviewForCandidate() {
    if (m_popupVisible && m_previewCandidateId &&
        m_previewCandidateId == m_keyboardHandler.GetActiveItemId() && !m_previewSuppressed) {
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
    ShowPreviewForItem(m_keyboardHandler.GetActiveItemId());
}

void MainWindow::HidePreview() {
    KillTimer(AppConstants::Timer::kPreview);
    ++m_previewGeneration;
    m_previewCandidateId = 0;
    m_previewItemId = 0;
    m_previewWorker.Hide();
}

void MainWindow::TogglePreview() {
    if (m_previewWorker.IsVisible()) {
        m_previewSuppressed = true;
        HidePreview();
    } else {
        m_previewSuppressed = false;
        ShowPreviewForSelection();
    }
}

void MainWindow::PasteItem(int index) {
    if (m_loadingList || m_pasteInProgress ||
        index < 0 || static_cast<size_t>(index) >= m_items.size()) {
        return;
    }
    const HWND target = m_pasteController.GetTargetWindow();
    const HWND target_focus = m_pasteController.GetTargetFocusWindow();
    const sqlite3_int64 id = m_items[static_cast<size_t>(index)].id;
    const auto [paste, plain] = ResolvePasteAction(
        m_settings.paste_by_default,
        m_settings.remove_formatting_by_default,
        (GetKeyState(VK_CONTROL) & 0x8000) != 0,
        (GetKeyState(VK_MENU) & 0x8000) != 0,
        (GetKeyState(VK_SHIFT) & 0x8000) != 0
    );
    m_pasteInProgress = true;
    try {
        m_storage.GetItemAsync(
            id,
            PayloadMode::Full,
            [this, id, target, target_focus, paste, plain](
                std::optional<ClipboardItem> item,
                std::string error
            ) mutable {
                m_pasteInProgress = false;
                bool success = error.empty() && item.has_value();
                if (!success && error.empty()) {
                    error = "剪贴板项目不存在";
                }
                if (success && !m_clipboard.WriteClipboardItem(*item, plain)) {
                    success = false;
                    error = "无法写入系统剪贴板";
                }
                if (success) {
                    try {
                        m_storage.MarkCopied(id);
                    } catch (const std::exception &exception) {
                        success = false;
                        error = exception.what();
                    } catch (...) {
                        success = false;
                        error = "无法更新剪贴板项目状态";
                    }
                }
                if (!success) {
                    ::MessageBoxA(
                        m_hWnd,
                        error.empty() ? "无法粘贴剪贴板项目" : error.c_str(),
                        "无法粘贴",
                        MB_OK | MB_ICONERROR
                    );
                    return;
                }
                HideMainWindow();
                m_pasteController.RestoreTargetFocusAndPaste(target, target_focus, paste);
                RequestUiUpdate(AppConstants::UiUpdate::kHistory);
            }
        );
    } catch (const std::exception &exception) {
        m_pasteInProgress = false;
        ::MessageBoxA(m_hWnd, exception.what(), "无法粘贴", MB_OK | MB_ICONERROR);
    } catch (...) {
        m_pasteInProgress = false;
        ::MessageBoxW(m_hWnd, L"无法粘贴剪贴板项目。", L"无法粘贴", MB_OK | MB_ICONERROR);
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
    const ClipboardItem item = m_items[static_cast<size_t>(index)];
    try {
        if (item.pinned) {
            m_storage.TogglePin(item.id, {}, false);
        } else {
            const std::wstring key = PinKeyPolicy::Next(m_storage.GetPinnedItems(), m_settings);
            if (key.empty()) {
                ::MessageBoxW(
                    m_hWnd,
                    L"没有可用的置顶快捷键，请先取消一个置顶项目。",
                    L"置顶",
                    MB_OK
                );
                return;
            }
            m_storage.TogglePin(item.id, key, true);
        }
        RequestUiUpdate(AppConstants::UiUpdate::kHistory);
    } catch (const std::exception &error) {
        ::MessageBoxA(m_hWnd, error.what(), "无法修改置顶", MB_OK | MB_ICONERROR);
    } catch (...) {
        ::MessageBoxW(m_hWnd, L"无法修改置顶项目。", L"无法修改置顶", MB_OK | MB_ICONERROR);
    }
}

void MainWindow::DeleteSelectedItem() {
    const int index = SelectedHistoryIndex();
    if (index < 0) {
        return;
    }
    const sqlite3_int64 item_id = m_items[static_cast<size_t>(index)].id;
    try {
        m_storage.DeleteItem(item_id);
        RequestUiUpdate(AppConstants::UiUpdate::kHistory);
    } catch (const std::exception &error) {
        ::MessageBoxA(m_hWnd, error.what(), "无法删除剪贴板项目", MB_OK | MB_ICONERROR);
    } catch (...) {
        ::MessageBoxW(m_hWnd, L"无法删除剪贴板项目。", L"无法删除剪贴板项目",
                      MB_OK | MB_ICONERROR);
    }
}

void MainWindow::ClearHistory(bool all) {
    const bool suppress = m_suppressClearAlert;
    bool remember = false;
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
        remember = checked != FALSE;
        if (remember) {
            m_suppressClearAlert = true;
        }
    }
    const bool clear_clipboard = m_settings.clear_system_clipboard;
    try {
        if (remember) {
            m_storage.SaveSuppressClearAlert(true);
        }
        if (all) {
            m_storage.DeleteAll();
        } else {
            m_storage.DeleteUnpinned();
        }
        if (remember) {
            m_suppressClearAlert = true;
        }
    } catch (const std::exception &error) {
        if (remember) m_suppressClearAlert = false;
        ::MessageBoxA(m_hWnd, error.what(), "无法清空历史", MB_OK | MB_ICONERROR);
        return;
    } catch (...) {
        if (remember) m_suppressClearAlert = false;
        ::MessageBoxW(m_hWnd, L"无法清空历史。", L"无法清空历史", MB_OK | MB_ICONERROR);
        return;
    }
    if (clear_clipboard && !m_clipboard.ClearClipboard()) {
        ::MessageBoxW(m_hWnd, L"无法清空系统剪贴板。", L"无法清空系统剪贴板",
                      MB_OK | MB_ICONERROR);
    }
    ::SetWindowTextW(m_search, L"");
    RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout);
}

void MainWindow::OpenAbout() {
    m_modalShowing = true;
    std::wstring about = L"maccy ";
    about += AppConstants::kAppVersion;
    about += L"\n\n轻量 Windows 剪贴板历史工具"
             L"\n这是一个独立的 Windows 项目，受 macOS 版 Maccy 启发。"
             L"\n本项目不是 Maccy 官方 Windows 版本，也不隶属于或代表 Maccy 官方项目。"
             L"\n布局和交互参考 Maccy 2.7.1。"
             L"\n部分视觉资源来自 Maccy 项目，按 MIT 许可证使用。"
             L"\n本项目源代码采用 MIT License，第三方组件和视觉资源遵循各自许可证。"
             L"\n\n使用 C++、WTL 和 SQLite 构建。";
    const std::wstring caption = L"关于 maccy";
    MessageBoxW(about.c_str(), caption.c_str(), MB_OK | MB_ICONINFORMATION);
    m_modalShowing = false;
}

void MainWindow::OpenSettings() {
    HideMainWindow();
    if (m_settingsWindow == nullptr) {
        m_settingsWindow = std::make_unique<SettingsWindow>(
            m_storage,
            m_hWnd,
            m_settings,
            m_ignoredLists,
            [this](const AppSettings &settings, std::uint32_t updates) {
                OnSettingsChanged(settings, updates);
            },
            [this]() {
                return StartUpdateCheck(UpdateCheckMode::Manual);
            }
        );
    } else {
        m_settingsWindow->SetStateSnapshot(m_settings, m_ignoredLists);
    }
    if (!m_settingsWindow->CreateOrShow()) {
        ::MessageBoxW(m_hWnd, L"无法打开设置窗口。", L"maccy", MB_OK | MB_ICONERROR);
    } else {
        m_settingsWindow->SetUpdateCheckBusy(m_updateChecker.IsChecking());
    }
}

bool MainWindow::StartUpdateCheck(UpdateCheckMode mode) {
    if (!m_updateChecker.Start(mode)) {
        return false;
    }
    if (m_settingsWindow != nullptr) {
        m_settingsWindow->SetUpdateCheckBusy(true);
    }
    return true;
}

void MainWindow::HandleUpdateCheckResult(const UpdateCheckResult &result) {
    if (m_settingsWindow != nullptr) {
        m_settingsWindow->SetUpdateCheckBusy(false);
    }
    HWND owner = m_hWnd;
    if (m_settingsWindow != nullptr && m_settingsWindow->IsOpen()) {
        owner = m_settingsWindow->Window();
    }

    if (!result.succeeded) {
        if (result.mode == UpdateCheckMode::Manual) {
            std::wstring message = L"检查更新失败。\n";
            message += result.error.empty() ? L"请稍后重试。" : result.error;
            ::MessageBoxW(owner, message.c_str(), L"检查更新", MB_OK | MB_ICONWARNING);
        } else if (!result.error.empty()) {
            ::OutputDebugStringW((L"Update check failed: " + result.error + L"\n").c_str());
        }
        return;
    }

    if (!result.update_available) {
        if (result.mode == UpdateCheckMode::Manual) {
            std::wstring message = L"当前已经是最新版本。\n当前版本：";
            message += DisplayVersion(result.current_version);
            ::MessageBoxW(owner, message.c_str(), L"检查更新", MB_OK | MB_ICONINFORMATION);
        }
        return;
    }

    const std::wstring message = L"发现新版本 " + DisplayVersion(result.latest_version) +
        L"。\n当前版本：" + DisplayVersion(result.current_version) +
        L"\n是否打开下载页面？";
    if (::MessageBoxW(owner, message.c_str(), L"检查更新", MB_YESNO | MB_ICONINFORMATION) == IDYES &&
        !OpenReleasePage(owner, result.release_url)) {
        ::MessageBoxW(
            owner,
            L"无法打开更新页面，请检查默认浏览器设置。",
            L"检查更新",
            MB_OK | MB_ICONERROR
        );
    }
}

void MainWindow::ExitApplication() {
    m_exiting = true;
    if (m_settings.clear_on_quit) {
        try {
            m_storage.DeleteUnpinned();
        } catch (...) {
        }
    }
    if (m_settings.clear_on_quit && m_settings.clear_system_clipboard) {
        m_clipboard.ClearClipboard();
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
        AppConstants::UI::kMinimumPopupWidth,
        AppConstants::UI::kMaximumPopupWidth
    );
    const int height = std::clamp(
        resized ? static_cast<int>(rect.bottom - rect.top) : m_settings.window_height,
        AppConstants::UI::kMinimumPopupHeight,
        AppConstants::UI::kMaximumPopupHeight
    );
    const bool persist_popup_position = m_activePopupPosition != PopupPosition::StatusItem;
    if (persist_popup_position) {
        m_settings.popup_x = rect.left;
        m_settings.popup_y = rect.top;
    }
    m_settings.window_width = width;
    m_settings.window_height = height;
    PersistSettings();
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
    // Invalidate outstanding results and queue a search as soon as the edit changes.
    ++m_historyGeneration;
    RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout);
}

void MainWindow::UpdateTrayTooltip() {
    ::lstrcpynW(m_notifyIcon.szTip, L"剪贴板历史", ARRAYSIZE(m_notifyIcon.szTip));
    if (m_trayIconAdded) {
        m_notifyIcon.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
        ::Shell_NotifyIconW(NIM_MODIFY, &m_notifyIcon);
    }
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
    const HICON icon = LoadTrayIcon(m_settings.menu_icon);
    if (icon == nullptr) {
        return;
    }
    const HICON previous_icon = m_trayIcon;
    m_trayIcon = icon;
    m_notifyIcon.hIcon = m_trayIcon;
    m_notifyIcon.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    UpdateTrayTooltip();
    if (previous_icon != nullptr) {
        DestroyIcon(previous_icon);
    }
}

void MainWindow::ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }
    m_trayMenuShowing = true;
    AppendMenuW(menu, MF_STRING, kTrayCommandShow, L"打开");
    AppendMenuW(menu, MF_STRING, kTrayCommandSettings, L"设置");
    AppendMenuW(menu, MF_STRING, kTrayCommandClear, L"清空");
    AppendMenuW(
        menu,
        MF_STRING | (m_settings.ignore_events ? MF_CHECKED : MF_UNCHECKED),
        kTrayCommandIgnore,
        L"暂停"
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
    if (m_trayIcon != nullptr) {
        DestroyIcon(m_trayIcon);
        m_trayIcon = nullptr;
    }
    m_notifyIcon.hIcon = nullptr;
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

std::uint32_t MainWindow::ApplySettings(
    const AppSettings &settings,
    std::uint32_t requestedUpdates
) {
    const AppSettings previous = m_settings;
    m_settings = settings;

    std::uint32_t updates = requestedUpdates &
        (AppConstants::UiUpdate::kHistory |
         AppConstants::UiUpdate::kLayout |
         AppConstants::UiUpdate::kTray |
         AppConstants::UiUpdate::kFooter);

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
        previous.show_in_status_bar != m_settings.show_in_status_bar) {
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

void MainWindow::PersistSettings() {
    m_storage.SaveSettings(m_settings);
}

void MainWindow::OnSettingsChanged(const AppSettings &settings, std::uint32_t requestedUpdates) {
    const std::uint32_t updates = ApplySettings(settings, requestedUpdates);
    if ((requestedUpdates & AppConstants::UiUpdate::kIgnoreRules) != 0) {
        m_ignoredLists = m_storage.LoadIgnoreLists();
        m_clipboard.ReloadIgnoreLists(m_ignoredLists);
        if (m_settingsWindow != nullptr) {
            m_settingsWindow->SetStateSnapshot(m_settings, m_ignoredLists);
        }
    }
    RequestUiUpdate(updates);
}

// Callback implementations for KeyboardHandler
void MainWindow::OnPreviewCallback(void* context, sqlite3_int64 itemId, bool) {
    auto* window = static_cast<MainWindow*>(context);
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
    m_updateChecker.SetWindow(m_hWnd);
    m_storage.SetUiWindow(m_hWnd);
    if (!m_historyRenderer.Initialize(m_hWnd) ||
        !BindControls()) {
        handled = FALSE;
        return FALSE;
    }

    m_pasteController.SetOwner(m_hWnd);
    if (!m_clipboard.Initialize(m_hWnd)) {
        ::MessageBoxW(m_hWnd, L"无法注册剪贴板监听。", L"maccy", MB_OK | MB_ICONERROR);
        handled = FALSE;
        return FALSE;
    }
    m_clipboard.ReloadIgnoreLists(m_ignoredLists);

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

    if (!m_isolated) {
        m_keyboardHandler.RegisterGlobalHotKey(AppConstants::HotKey::kOpenPopup);
    }
    RequestUiUpdate(AppConstants::UiUpdate::kHistory | AppConstants::UiUpdate::kLayout |
                    AppConstants::UiUpdate::kTray);
    if (!m_isolated && m_settings.check_for_updates) {
        StartUpdateCheck(UpdateCheckMode::Automatic);
    }
    return TRUE;
}

LRESULT MainWindow::OnSize(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    LayoutHistoryControls();
    if (m_previewWorker.IsVisible()) {
        m_previewWorker.Reposition();
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
    if (m_previewWorker.IsVisible()) {
        m_previewWorker.Reposition();
    }
    return 0;
}

LRESULT MainWindow::OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    SaveWindowGeometry(true);
    if (m_previewWorker.IsVisible()) {
        m_previewWorker.Reposition();
    }
    return 0;
}

LRESULT MainWindow::OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL& handled) {
    handled = TRUE;
    return 0;
}

LRESULT MainWindow::OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
    if (info == nullptr) {
        handled = FALSE;
        return 0;
    }
    info->ptMinTrackSize.x = AppConstants::UI::kMinimumPopupWidth;
    info->ptMinTrackSize.y = AppConstants::UI::kMinimumPopupHeight;
    info->ptMaxTrackSize.x = AppConstants::UI::kMaximumPopupWidth;
    info->ptMaxTrackSize.y = AppConstants::UI::kMaximumPopupHeight;
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
    POINT clientPoint = point;
    ::ScreenToClient(m_hWnd, &clientPoint);
    if (IsSearchClearHit(clientPoint)) {
        handled = TRUE;
        return HTCLIENT;
    }

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

LRESULT MainWindow::OnLButtonDown(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    if (!IsSearchClearHit(point)) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    m_searchClearPressed = true;
    ::SetCapture(m_hWnd);
    return 0;
}

LRESULT MainWindow::OnLButtonUp(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    if (!m_searchClearPressed) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
    const bool activate = IsSearchClearHit(point);
    m_searchClearPressed = false;
    if (::GetCapture() == m_hWnd) {
        ::ReleaseCapture();
    }
    if (activate) {
        ClearSearch();
    }
    return 0;
}

LRESULT MainWindow::OnCaptureChanged(UINT, WPARAM, LPARAM, BOOL& handled) {
    m_searchClearPressed = false;
    handled = TRUE;
    return 0;
}

LRESULT MainWindow::OnSetCursor(UINT, WPARAM, LPARAM lParam, BOOL& handled) {
    if (LOWORD(lParam) == HTCLIENT) {
        POINT point{};
        if (::GetCursorPos(&point) && ::ScreenToClient(m_hWnd, &point) &&
            IsSearchClearHit(point)) {
            ::SetCursor(::LoadCursorW(nullptr, IDC_HAND));
            handled = TRUE;
            return TRUE;
        }
    }
    handled = FALSE;
    return 0;
}

LRESULT MainWindow::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(&paint);
    RECT client{};
    GetClientRect(&client);
    const bool showSearchClear = m_search != nullptr &&
        !ReadWindowText(m_search).empty() && m_searchHeader.showSearch;
    m_historyRenderer.OnPaint(dc, client, m_pinSeparatorY, m_footerSeparatorY,
        m_searchHeader, showSearchClear);
    EndPaint(&paint);
    return 0;
}

LRESULT MainWindow::OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&) {
    return 1;
}

LRESULT MainWindow::OnSearchEditColor(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled) {
    handled = reinterpret_cast<HWND>(lParam) == m_search;
    if (!handled) return 0;
    HDC dc = reinterpret_cast<HDC>(wParam);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(dc, GetSysColor(COLOR_BTNFACE));
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
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
    if (LOWORD(wParam) == WA_INACTIVE) {
        HandlePopupActivation(reinterpret_cast<HWND>(lParam));
    }
    return 0;
}

LRESULT MainWindow::OnPopupActivation(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled) {
    handled = TRUE;
    if (LOWORD(wParam) == WA_INACTIVE) {
        HandlePopupActivation(reinterpret_cast<HWND>(lParam));
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

    if (notification == BN_CLICKED &&
        (command == IDC_PREVIEW_PIN || command == IDC_PREVIEW_DELETE)) {
        handled = TRUE;
        const sqlite3_int64 item_id = static_cast<sqlite3_int64>(lParam);
        const auto item = std::find_if(m_items.begin(), m_items.end(), [item_id](const ClipboardItem &candidate) {
            return candidate.id == item_id;
        });
        if (item == m_items.end()) {
            return 0;
        }

        const auto item_index = static_cast<int>(std::distance(m_items.begin(), item));
        m_keyboardHandler.SetActiveHistoryItem(item_index, m_items, m_historyList, m_pinsList);
        if (command == IDC_PREVIEW_PIN) ToggleSelectedPin();
        else DeleteSelectedItem();
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
        PersistSettings();
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
        if (m_searchHeader.showSearch) {
            ::InvalidateRect(m_hWnd, &m_searchHeader.searchClear, FALSE);
        }
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
                    if (!selected_by_mouse) {
                        m_keyboardHandler.ClearHistoryHover();
                        if (m_previewWorker.IsVisible()) {
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
        m_pasteController.OnPasteTimer();
        return 0;
    }
    if (wParam == AppConstants::Timer::kSearchResultCommit) {
        handled = TRUE;
        if (HasPendingKeyboardInput(m_search) &&
            ::SetTimer(m_hWnd, AppConstants::Timer::kSearchResultCommit,
                       USER_TIMER_MINIMUM, nullptr) != 0) {
            return 0;
        }
        ApplyDeferredHistoryResult();
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
            PersistSettings();
        } else {
            ShowMainWindow(PopupPosition::StatusItem);
        }
        break;
    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        m_pasteController.CaptureTargetWindow();
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

LRESULT MainWindow::OnClipboardUpdate(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    const bool previous_ignore_events = m_settings.ignore_events;
    const bool previous_ignore_only = m_settings.ignore_only_next_event;
    try {
        m_clipboard.OnClipboardUpdate();
    } catch (const std::exception &error) {
        ::OutputDebugStringA(error.what());
        ::OutputDebugStringA("\n");
    } catch (...) {
        ::OutputDebugStringA("Unhandled clipboard update exception\n");
    }
    if (previous_ignore_events != m_settings.ignore_events ||
        previous_ignore_only != m_settings.ignore_only_next_event) {
        PersistSettings();
        RequestUiUpdate(AppConstants::UiUpdate::kFooter | AppConstants::UiUpdate::kTray);
    }
    return 0;
}

LRESULT MainWindow::OnPreviewWorkerResult(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    m_previewWorker.DrainUiCallbacks();
    return 0;
}

LRESULT MainWindow::OnStorageWorkerResult(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    m_storage.DrainUiCallbacks();
    return 0;
}

LRESULT MainWindow::OnUpdateCheckerResult(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    for (const auto &result : m_updateChecker.TakeResults()) {
        HandleUpdateCheckResult(result);
    }
    return 0;
}

LRESULT MainWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
    m_popupVisible = false;
    m_updateChecker.Stop();
    KillTimer(AppConstants::Timer::kSearchResultCommit);
    m_deferredHistoryResult.reset();
    KillTimer(AppConstants::Timer::kPreview);
    m_pasteController.StopPasteTimer();
    m_keyboardHandler.Shutdown();
    m_previewWorker.SetUiWindow(nullptr);
    m_clipboard.Shutdown();
    m_clipboard.SetSaveCallback({});
    m_storage.SetUiWindow(nullptr);
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
    m_trayIcon = LoadTrayIcon(m_settings.menu_icon);
    if (m_trayIcon == nullptr) {
        return false;
    }
    m_notifyIcon = {};
    m_notifyIcon.cbSize = sizeof(m_notifyIcon);
    m_notifyIcon.hWnd = m_hWnd;
    m_notifyIcon.uID = kTrayIconId;
    m_notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_notifyIcon.uCallbackMessage = AppConstants::kTrayIconMessage;
    m_notifyIcon.hIcon = m_trayIcon;
    UpdateTrayTooltip();
    m_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &m_notifyIcon) == TRUE;
    if (!m_trayIconAdded) {
        DestroyIcon(m_trayIcon);
        m_trayIcon = nullptr;
        m_notifyIcon.hIcon = nullptr;
    }
    return m_trayIconAdded;
}

void MainWindow::ShowMainWindow() {
    ShowMainWindow(m_settings.popup_position);
}

void MainWindow::ShowMainWindow(PopupPosition popup_position) {
    m_activePopupPosition = popup_position;
    m_keyboardHandler.ClearHistoryHover();
    m_pasteController.CaptureTargetWindow();
    m_previewSuppressed = false;
    ::SetWindowTextW(m_search, L"");
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
