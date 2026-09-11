#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <cwctype>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <memory>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Database.h"
#include "PreviewWindow.h"
#include "Settings.h"
#include "SettingsWindow.h"

CAppModule _Module;

namespace {

constexpr UINT kTrayIconMessage = WM_APP + 1;
constexpr UINT kTrayIconId = 1;
constexpr UINT kTrayCommandShow = 1001;
constexpr UINT kTrayCommandSettings = 1002;
constexpr UINT kTrayCommandClear = 1003;
constexpr UINT kTrayCommandIgnore = 1004;
constexpr UINT kTrayCommandExit = 1005;
constexpr UINT kHotkeyId = 1006;
constexpr UINT_PTR kSearchTimerId = 1;
constexpr UINT_PTR kPreviewTimerId = 2;
constexpr UINT kSearchDebounceMilliseconds = 180;

constexpr int kSearchControlId = IDC_HISTORY_SEARCH;
constexpr int kHistoryListControlId = IDC_HISTORY_LIST;

// Maccy's default history panel is about 450 px wide. Keep the Windows
// history panel compact as well so the list remains easy to scan beside the
// independent preview window.
constexpr int kPopupWidth = 450;
constexpr int kPopupHeight = 520;
constexpr int kHistoryWindowMargin = 8;
constexpr int kHistorySearchGap = 6;
constexpr int kHistoryFallbackSearchHeight = 24;
constexpr size_t kMaximumClipboardCharacters = 1024 * 1024;
constexpr size_t kMaximumClipboardBytes = 32 * 1024 * 1024;

constexpr wchar_t kControlOwnerProperty[] = L"ClipboardMainWindow";

std::filesystem::path GetDatabasePath() {
    PWSTR local_app_data = nullptr;
    const HRESULT result = SHGetKnownFolderPath(
        FOLDERID_LocalAppData,
        KF_FLAG_DEFAULT,
        nullptr,
        &local_app_data
    );
    if (FAILED(result)) {
        throw std::runtime_error("Unable to locate the LocalAppData folder");
    }

    const std::filesystem::path path =
        std::filesystem::path(local_app_data) / L"Clipboard" / L"app.db";
    CoTaskMemFree(local_app_data);
    return path;
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

std::wstring Lower(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (const wchar_t character : value) {
        result.push_back(static_cast<wchar_t>(std::towlower(character)));
    }
    return result;
}

std::wstring Trim(std::wstring value) {
    const auto is_space = [](wchar_t character) { return std::iswspace(character) != 0; };
    const auto first = std::find_if_not(value.begin(), value.end(), is_space);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    if (first >= last) {
        return {};
    }
    return std::wstring(first, last);
}

std::wstring MakeTitle(std::wstring value, bool show_special_symbols) {
    value.resize(std::min<size_t>(value.size(), 1000));
    if (!show_special_symbols) {
        return Trim(std::move(value));
    }

    size_t leading = 0;
    while (leading < value.size() && value[leading] == L' ') {
        value[leading++] = L'\x00b7';
    }
    size_t trailing = value.size();
    while (trailing > 0 && value[trailing - 1] == L' ') {
        value[--trailing] = L'\x00b7';
    }

    std::wstring result;
    result.reserve(value.size() + 8);
    for (const wchar_t character : value) {
        switch (character) {
        case L'\r':
            break;
        case L'\n':
            result += L'\x23ce';
            break;
        case L'\t':
            result += L'\x21e5';
            break;
        default:
            result += character;
            break;
        }
    }
    return result;
}

std::wstring PreviewText(std::wstring_view text) {
    constexpr size_t kPreviewCharacters = 180;
    std::wstring preview;
    preview.reserve(std::min(text.size(), kPreviewCharacters + 3));
    for (const wchar_t character : text) {
        if (preview.size() >= kPreviewCharacters) {
            preview += L"...";
            break;
        }
        preview += (character < L' ' && character != L'\t') ? L' ' : character;
    }
    return preview;
}

bool EqualInsensitive(std::wstring_view lhs, std::wstring_view rhs) {
    return Lower(lhs) == Lower(rhs);
}

std::wstring FormatName(UINT format) {
    switch (format) {
    case CF_UNICODETEXT:
        return L"CF_UNICODETEXT";
    case CF_TEXT:
        return L"CF_TEXT";
    case CF_HDROP:
        return L"CF_HDROP";
    case CF_DIB:
        return L"CF_DIB";
    case CF_DIBV5:
        return L"CF_DIBV5";
    case CF_BITMAP:
        return L"CF_BITMAP";
    default:
        break;
    }

    std::array<wchar_t, 256> name{};
    const int length = GetClipboardFormatNameW(format, name.data(), static_cast<int>(name.size()));
    if (length > 0) {
        return std::wstring(name.data(), static_cast<size_t>(length));
    }
    return L"FORMAT_" + std::to_wstring(format);
}

bool IsHtmlOrRtf(std::wstring_view name) {
    return EqualInsensitive(name, L"HTML Format") || EqualInsensitive(name, L"Rich Text Format");
}

bool IsImageFormat(UINT format, std::wstring_view name) {
    return format == CF_DIB || format == CF_DIBV5 || format == CF_BITMAP ||
        EqualInsensitive(name, L"PNG") || EqualInsensitive(name, L"image/png") ||
        EqualInsensitive(name, L"JFIF") || EqualInsensitive(name, L"image/jpeg") ||
        EqualInsensitive(name, L"TIFF") || EqualInsensitive(name, L"image/tiff") ||
        EqualInsensitive(name, L"HEIC") || EqualInsensitive(name, L"image/heic");
}

bool IsTextFormat(UINT format, std::wstring_view name) {
    return format == CF_UNICODETEXT || format == CF_TEXT || IsHtmlOrRtf(name);
}

std::wstring GetSourceApplication() {
    HWND owner = GetClipboardOwner();
    if (owner == nullptr) {
        owner = GetOpenClipboardWindow();
    }
    if (owner == nullptr) {
        return {};
    }
    DWORD process_id = 0;
    GetWindowThreadProcessId(owner, &process_id);
    if (process_id == 0) {
        return {};
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process == nullptr) {
        return {};
    }
    std::array<wchar_t, 32768> path{};
    DWORD length = static_cast<DWORD>(path.size());
    const BOOL ok = QueryFullProcessImageNameW(process, 0, path.data(), &length);
    CloseHandle(process);
    return ok ? std::wstring(path.data(), length) : std::wstring{};
}

std::wstring NormalizePath(std::wstring value) {
    value = Lower(Trim(std::move(value)));
    while (!value.empty() && (value.back() == L'\\' || value.back() == L'/')) {
        value.pop_back();
    }
    return value;
}

bool MatchesApplication(std::wstring_view actual, std::wstring_view configured) {
    const std::wstring actual_path = NormalizePath(std::wstring(actual));
    const std::wstring configured_path = NormalizePath(std::wstring(configured));
    if (actual_path.empty() || configured_path.empty()) {
        return false;
    }
    if (actual_path == configured_path) {
        return true;
    }
    const size_t separator = configured_path.find_last_of(L"\\/");
    return separator == std::wstring::npos &&
        actual_path.substr(actual_path.find_last_of(L"\\/") + 1) == configured_path;
}

std::wstring ExtractClipboardText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_UNICODETEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    const SIZE_T capacity = storage_bytes / sizeof(wchar_t);
    if (capacity == 0 || capacity > kMaximumClipboardCharacters + 1) {
        return {};
    }
    const auto *text = static_cast<const wchar_t *>(GlobalLock(data));
    if (text == nullptr) {
        return {};
    }
    size_t length = 0;
    while (length < capacity && text[length] != L'\0') {
        ++length;
    }
    std::wstring result;
    if (length < capacity && length <= kMaximumClipboardCharacters) {
        result.assign(text, length);
    }
    GlobalUnlock(data);
    return result;
}

std::wstring ExtractClipboardAnsiText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_TEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    if (storage_bytes == 0 || storage_bytes > kMaximumClipboardCharacters + 1) {
        return {};
    }
    const auto *source = static_cast<const char *>(GlobalLock(data));
    if (source == nullptr) {
        return {};
    }
    size_t length = 0;
    while (length < storage_bytes && source[length] != '\0') {
        ++length;
    }
    std::wstring result;
    if (length > 0 && length <= kMaximumClipboardCharacters) {
        const int wide_length = MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            source,
            static_cast<int>(length),
            nullptr,
            0
        );
        if (wide_length > 0) {
            result.resize(static_cast<size_t>(wide_length));
            MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                source,
                static_cast<int>(length),
                result.data(),
                wide_length
            );
        }
    }
    GlobalUnlock(data);
    return result;
}

std::wstring FilesPreview(HGLOBAL data) {
    if (data == nullptr) {
        return {};
    }
    const HDROP drop = static_cast<HDROP>(data);
    const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    std::wstring result;
    for (UINT index = 0; index < count; ++index) {
        const UINT length = DragQueryFileW(drop, index, nullptr, 0);
        std::wstring path(static_cast<size_t>(length) + 1, L'\0');
        DragQueryFileW(drop, index, path.data(), length + 1);
        path.resize(length);
        if (!result.empty()) {
            result += L"; ";
        }
        result += path;
    }
    return result;
}

std::wstring HashCapture(const std::vector<ClipboardFormatData> &data) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto add = [&hash](const unsigned char *bytes, size_t count) {
        for (size_t index = 0; index < count; ++index) {
            hash ^= bytes[index];
            hash *= 1099511628211ULL;
        }
    };
    for (const ClipboardFormatData &item : data) {
        add(reinterpret_cast<const unsigned char *>(item.name.data()), item.name.size() * sizeof(wchar_t));
        add(reinterpret_cast<const unsigned char *>(&item.format), sizeof(item.format));
        add(item.bytes.data(), item.bytes.size());
    }
    std::wstringstream stream;
    stream << std::hex << std::setw(16) << std::setfill(L'0') << hash;
    return stream.str();
}

bool SameHotKey(const HotKeyConfig &lhs, const HotKeyConfig &rhs) {
    return lhs.modifiers == rhs.modifiers && lhs.virtual_key == rhs.virtual_key;
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

} // namespace

class MainWindow : public CDialogImpl<MainWindow> {
public:
    enum { IDD = IDD_HISTORY };

    explicit MainWindow(Database &database)
        : m_database(database), m_settings(AppSettings::Load(database)) {}

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_CLIPBOARDUPDATE, OnClipboardUpdate)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotKey)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(kTrayIconMessage, OnTrayIcon)
        MESSAGE_HANDLER(kSettingsChangedMessage, OnSettingsChanged)
    END_MSG_MAP()

    bool AddTrayIcon() {
        if (!m_settings.show_in_status_bar) {
            RemoveTrayIcon();
            // A deliberately hidden tray icon is a valid configuration, not
            // an Explorer registration failure. The global hotkey remains
            // available to reopen the popup.
            return true;
        }
        m_notifyIcon = {};
        m_notifyIcon.cbSize = sizeof(m_notifyIcon);
        m_notifyIcon.hWnd = m_hWnd;
        m_notifyIcon.uID = kTrayIconId;
        m_notifyIcon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
        m_notifyIcon.uCallbackMessage = kTrayIconMessage;
        m_notifyIcon.hIcon = TrayIconForName(m_settings.menu_icon);
        UpdateTrayTooltip();
        m_trayIconAdded = Shell_NotifyIconW(NIM_ADD, &m_notifyIcon) == TRUE;
        return m_trayIconAdded;
    }

    void ShowMainWindow() {
        CaptureTargetWindow();
        ::SetWindowTextW(m_search, L"");
        m_searchQuery.clear();
        KillTimer(kSearchTimerId);
        HidePreview();
        RefreshHistory(L"");
        PositionPopup();

        m_popupVisible = true;
        ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hWnd);
        if (m_settings.show_search && m_settings.search_visibility == SearchVisibility::Always) {
            SetHistorySearchVisible(true);
            ::SetFocus(m_search);
            SendMessageW(m_search, EM_SETSEL, 0, -1);
        } else {
            ::SetFocus(m_hWnd);
        }
        UpdateWindow();
        UpdateHistoryHoverFromCursor();
    }

private:
    static LRESULT CALLBACK SearchWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto *owner = reinterpret_cast<MainWindow *>(GetPropW(window, kControlOwnerProperty));
        const WNDPROC original = owner != nullptr && owner->m_originalSearchProc != nullptr
            ? owner->m_originalSearchProc
            : ::DefWindowProcW;
        if (owner != nullptr && message == WM_KEYDOWN) {
            if (wParam == VK_ESCAPE) {
                owner->HideMainWindow();
                return 0;
            }
            if (wParam == VK_RETURN) {
                owner->PasteSelectedItem();
                return 0;
            }
            if (wParam == VK_UP || wParam == VK_DOWN) {
                owner->NavigateHistoryFromSearch(wParam == VK_DOWN);
                return 0;
            }
            if (owner->HandlePopupShortcut(wParam)) {
                return 0;
            }
        }
        return CallWindowProcW(original, window, message, wParam, lParam);
    }

    static LRESULT CALLBACK HistoryListWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto *owner = reinterpret_cast<MainWindow *>(GetPropW(window, kControlOwnerProperty));
        const WNDPROC original = owner != nullptr && owner->m_originalHistoryListProc != nullptr
            ? owner->m_originalHistoryListProc
            : ::DefWindowProcW;
        if (owner != nullptr && message == WM_MOUSEMOVE) {
            owner->OnHistoryMouseMove(
                window,
                POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)}
            );
        }
        if (owner != nullptr && message == WM_MOUSELEAVE) {
            owner->OnHistoryMouseLeave();
        }
        if (owner != nullptr && message == WM_KEYDOWN) {
            if (wParam == VK_ESCAPE) {
                owner->HideMainWindow();
                return 0;
            }
            if (wParam == VK_RETURN) {
                owner->PasteSelectedItem();
                return 0;
            }
            if (owner->HandlePopupShortcut(wParam)) {
                return 0;
            }
        }
        if (owner != nullptr && (message == WM_MOUSEWHEEL || message == WM_VSCROLL)) {
            const LRESULT result = CallWindowProcW(original, window, message, wParam, lParam);
            owner->UpdateHistoryHoverFromCursor();
            return result;
        }
        if (owner != nullptr && message == WM_LBUTTONUP) {
            const POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            const LRESULT hit = SendMessageW(
                window,
                LB_ITEMFROMPOINT,
                0,
                MAKELPARAM(point.x, point.y)
            );
            const bool clicked_item = HIWORD(hit) == 0;
            const int item_index = LOWORD(hit);
            const LRESULT result = CallWindowProcW(original, window, message, wParam, lParam);
            if (clicked_item) {
                owner->PasteItem(item_index);
            }
            return result;
        }
        return CallWindowProcW(original, window, message, wParam, lParam);
    }

    bool BindControls() {
        m_search = ::GetDlgItem(m_hWnd, kSearchControlId);
        m_historyList = ::GetDlgItem(m_hWnd, kHistoryListControlId);
        if (m_search == nullptr || m_historyList == nullptr) {
            return false;
        }

        SetPropW(m_search, kControlOwnerProperty, reinterpret_cast<HANDLE>(this));
        SetPropW(m_historyList, kControlOwnerProperty, reinterpret_cast<HANDLE>(this));
        m_originalSearchProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            m_search,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&MainWindow::SearchWindowProc)
        ));
        m_originalHistoryListProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(
            m_historyList,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&MainWindow::HistoryListWindowProc)
        ));

        const HFONT font = m_normalFont != nullptr
            ? m_normalFont
            : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        for (HWND control : {m_search, m_historyList}) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
        SendMessageW(m_search, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"搜索剪贴板内容…"));
        RECT search_rect{};
        if (::GetWindowRect(m_search, &search_rect)) {
            m_searchHeight = std::max(1L, search_rect.bottom - search_rect.top);
        }
        return true;
    }

    void LayoutHistoryControls() {
        if (m_search == nullptr || m_historyList == nullptr || !::IsWindow(m_hWnd)) {
            return;
        }

        RECT client{};
        ::GetClientRect(m_hWnd, &client);
        const int width = std::max(1L, client.right - client.left);
        const int height = std::max(1L, client.bottom - client.top);
        const int control_width = std::max(1, width - 2 * kHistoryWindowMargin);
        const bool show_search = ::IsWindowVisible(m_search) != FALSE;

        int list_top = kHistoryWindowMargin;
        if (show_search) {
            const int search_height = std::max(
                kHistoryFallbackSearchHeight,
                m_searchHeight
            );
            ::SetWindowPos(
                m_search,
                nullptr,
                kHistoryWindowMargin,
                kHistoryWindowMargin,
                control_width,
                search_height,
                SWP_NOZORDER | SWP_NOACTIVATE
            );
            list_top += search_height + kHistorySearchGap;
        }

        const int list_height = std::max(
            1,
            height - list_top - kHistoryWindowMargin
        );
        ::SetWindowPos(
            m_historyList,
            nullptr,
            kHistoryWindowMargin,
            list_top,
            control_width,
            list_height,
            SWP_NOZORDER | SWP_NOACTIVATE
        );
    }

    void ApplyHistoryVisibility() {
        if (m_search == nullptr) {
            return;
        }
        const bool show_search = m_settings.show_search &&
            (m_settings.search_visibility == SearchVisibility::Always || !m_searchQuery.empty());
        SetHistorySearchVisible(show_search);
        ::ShowWindow(m_historyList, SW_SHOW);
    }

    void SetHistorySearchVisible(bool visible) {
        if (m_search == nullptr || !::IsWindow(m_search)) {
            return;
        }
        ::ShowWindow(m_search, visible ? SW_SHOW : SW_HIDE);
        LayoutHistoryControls();
    }

    void RestoreControlSubclass(HWND control, WNDPROC original) {
        if (control == nullptr || original == nullptr || !::IsWindow(control)) {
            return;
        }
        ::SetWindowLongPtrW(control, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(original));
        RemovePropW(control, kControlOwnerProperty);
    }

    void RestoreControlSubclasses() {
        RestoreControlSubclass(m_search, m_originalSearchProc);
        RestoreControlSubclass(m_historyList, m_originalHistoryListProc);
        m_originalSearchProc = nullptr;
        m_originalHistoryListProc = nullptr;
    }

    bool IsOurWindow(HWND window) const {
        if (window == nullptr) {
            return false;
        }
        return window == m_hWnd || window == m_search || window == m_historyList ||
            ::IsChild(m_hWnd, window) || m_previewWindow.ContainsWindow(window) ||
            (m_settingsWindow != nullptr && window == m_settingsWindow->Window());
    }

    void PositionOnMonitor(HMONITOR monitor, bool center) {
        MONITORINFO monitor_info{sizeof(monitor_info)};
        if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
            return;
        }
        const RECT &work_area = monitor_info.rcWork;
        int x = work_area.left;
        int y = work_area.top;
        if (center) {
            x = work_area.left + ((work_area.right - work_area.left) - kPopupWidth) / 2;
            y = work_area.top + ((work_area.bottom - work_area.top) - kPopupHeight) / 2;
        }
        const LONG max_x = std::max<LONG>(work_area.left, work_area.right - kPopupWidth);
        const LONG max_y = std::max<LONG>(work_area.top, work_area.bottom - kPopupHeight);
        x = std::clamp(x, static_cast<int>(work_area.left), static_cast<int>(max_x));
        y = std::clamp(y, static_cast<int>(work_area.top), static_cast<int>(max_y));
        ::SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, kPopupWidth, kPopupHeight, SWP_NOACTIVATE);
    }

    HMONITOR SelectedMonitor() const {
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
                auto *search = reinterpret_cast<MonitorSearch *>(data);
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

    void PositionPopup() {
        POINT cursor{};
        GetCursorPos(&cursor);
        const HMONITOR monitor = SelectedMonitor();
        MONITORINFO monitor_info{sizeof(monitor_info)};
        if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
            return;
        }
        const RECT &work_area = monitor_info.rcWork;
        int x = cursor.x;
        int y = cursor.y - kPopupHeight;

        switch (m_settings.popup_position) {
        case PopupPosition::WindowCenter: {
            RECT target{};
            if (::GetWindowRect(m_targetWindow, &target)) {
                x = target.left + ((target.right - target.left) - kPopupWidth) / 2;
                y = target.top + ((target.bottom - target.top) - kPopupHeight) / 2;
            } else {
                x = work_area.left + ((work_area.right - work_area.left) - kPopupWidth) / 2;
                y = work_area.top + ((work_area.bottom - work_area.top) - kPopupHeight) / 2;
            }
            break;
        }
        case PopupPosition::ScreenCenter:
            x = work_area.left + ((work_area.right - work_area.left) - kPopupWidth) / 2;
            y = work_area.top + ((work_area.bottom - work_area.top) - kPopupHeight) / 2;
            break;
        case PopupPosition::LastPosition:
            if (m_settings.popup_x != 0 || m_settings.popup_y != 0) {
                x = m_settings.popup_x;
                y = m_settings.popup_y;
            } else {
                x = work_area.left + ((work_area.right - work_area.left) - kPopupWidth) / 2;
                y = work_area.top + ((work_area.bottom - work_area.top) - kPopupHeight) / 2;
            }
            break;
        case PopupPosition::StatusItem: {
            NOTIFYICONIDENTIFIER identifier{};
            identifier.cbSize = sizeof(identifier);
            identifier.hWnd = m_hWnd;
            identifier.uID = kTrayIconId;
            RECT tray_rect{};
            if (m_trayIconAdded && SUCCEEDED(Shell_NotifyIconGetRect(&identifier, &tray_rect))) {
                x = tray_rect.left + ((tray_rect.right - tray_rect.left) - kPopupWidth) / 2;
                y = tray_rect.top - kPopupHeight - 6;
            }
            break;
        }
        case PopupPosition::Cursor:
        default:
            break;
        }

        const LONG max_x = std::max<LONG>(work_area.left, work_area.right - kPopupWidth);
        const LONG max_y = std::max<LONG>(work_area.top, work_area.bottom - kPopupHeight);
        x = std::clamp(x, static_cast<int>(work_area.left), static_cast<int>(max_x));
        y = std::clamp(y, static_cast<int>(work_area.top), static_cast<int>(max_y));
        ::SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, kPopupWidth, kPopupHeight, SWP_NOACTIVATE);
    }

    void PositionPreviewWindow() {
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

        const RECT &work_area = monitor_info.rcWork;
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
            // On a narrow monitor, keep the independent preview from
            // covering the history panel (especially its search field).
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

    void CaptureTargetWindow(HWND candidate = nullptr) {
        HWND foreground = candidate != nullptr ? candidate : GetForegroundWindow();
        if (foreground == nullptr || m_trayMenuShowing || IsOurWindow(foreground)) {
            return;
        }
        const HWND root = GetAncestor(foreground, GA_ROOT);
        if (root != nullptr) {
            foreground = root;
        }
        if (IsOurWindow(foreground)) {
            return;
        }

        m_targetWindow = foreground;
        m_targetFocusWindow = foreground;
        m_targetCaretRect = {};
        m_hasTargetCaretRect = false;
        const DWORD target_thread = GetWindowThreadProcessId(foreground, nullptr);
        GUITHREADINFO gui_info{sizeof(gui_info)};
        if (target_thread != 0 && GetGUIThreadInfo(target_thread, &gui_info)) {
            if (gui_info.hwndFocus != nullptr) {
                m_targetFocusWindow = gui_info.hwndFocus;
            }
            if (gui_info.hwndCaret != nullptr && !IsRectEmpty(&gui_info.rcCaret)) {
                m_targetCaretRect = gui_info.rcCaret;
                m_hasTargetCaretRect = true;
            }
        }
    }

    void ReloadIgnoreLists() {
        m_ignoredApps = m_database.GetList(DatabaseList::IgnoredApplications);
        m_ignoredFormats = m_database.GetList(DatabaseList::IgnoredFormats);
        m_ignoredRegexps = m_database.GetList(DatabaseList::IgnoredRegexps);
    }

    bool ShouldIgnoreApplication(std::wstring_view application) const {
        if (m_settings.ignore_all_apps_except_listed) {
            if (application.empty()) {
                return true;
            }
            return std::none_of(
                m_ignoredApps.begin(),
                m_ignoredApps.end(),
                [&application](const std::wstring &configured) {
                    return MatchesApplication(application, configured);
                }
            );
        }
        return std::any_of(
            m_ignoredApps.begin(),
            m_ignoredApps.end(),
            [&application](const std::wstring &configured) {
                return MatchesApplication(application, configured);
            }
        );
    }

    bool ShouldIgnoreFormat(std::wstring_view format) const {
        return std::any_of(
            m_ignoredFormats.begin(),
            m_ignoredFormats.end(),
            [&format](const std::wstring &configured) {
                return EqualInsensitive(format, configured);
            }
        );
    }

    bool ShouldIgnoreText(std::wstring_view text) const {
        for (const std::wstring &pattern : m_ignoredRegexps) {
            try {
                const std::wregex expression{pattern};
                if (std::regex_search(text.begin(), text.end(), expression)) {
                    return true;
                }
            } catch (const std::regex_error &) {
                // Match Maccy's behavior: an invalid rule is not allowed to
                // disable all clipboard monitoring.
            }
        }
        return false;
    }

    std::optional<ClipboardCapture> CaptureClipboard() const {
        const std::wstring application = GetSourceApplication();
        if (ShouldIgnoreApplication(application)) {
            return std::nullopt;
        }

        std::wstring text;
        std::vector<std::wstring> format_names;
        for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
            format_names.push_back(FormatName(format));
        }
        if (std::any_of(
            format_names.begin(),
            format_names.end(),
            [this](const std::wstring &name) { return ShouldIgnoreFormat(name); }
        )) {
            return std::nullopt;
        }

        if (m_settings.save_text && IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            text = ExtractClipboardText();
            if (text.empty() && IsClipboardFormatAvailable(CF_TEXT)) {
                text = ExtractClipboardAnsiText();
            }
        } else if (IsClipboardFormatAvailable(CF_UNICODETEXT) ||
                   IsClipboardFormatAvailable(CF_TEXT)) {
            // Regex ignore rules still apply when text storage is disabled.
            text = IsClipboardFormatAvailable(CF_UNICODETEXT)
                ? ExtractClipboardText()
                : ExtractClipboardAnsiText();
        }
        if (!text.empty() && ShouldIgnoreText(text)) {
            return std::nullopt;
        }

        ClipboardCapture capture;
        capture.application = application;
        capture.has_text = false;
        capture.has_image = false;
        capture.has_files = false;
        std::wstring file_preview;
        for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
            const std::wstring name = FormatName(format);
            const bool is_text = IsTextFormat(format, name);
            const bool is_image = IsImageFormat(format, name);
            const bool is_files = format == CF_HDROP;
            const bool enabled = (is_text && m_settings.save_text) ||
                (is_image && m_settings.save_images) ||
                (is_files && m_settings.save_files);
            if (!enabled || format == CF_BITMAP) {
                continue;
            }

            const HGLOBAL handle = static_cast<HGLOBAL>(GetClipboardData(format));
            if (handle == nullptr) {
                continue;
            }
            const SIZE_T bytes = GlobalSize(handle);
            if (bytes == 0 || bytes > kMaximumClipboardBytes) {
                continue;
            }
            const auto *source = static_cast<const unsigned char *>(GlobalLock(handle));
            if (source == nullptr) {
                continue;
            }
            ClipboardFormatData data;
            data.name = name;
            data.format = format;
            data.bytes.assign(source, source + bytes);
            GlobalUnlock(handle);
            if (is_text) {
                capture.has_text = true;
            }
            if (is_image) {
                capture.has_image = true;
            }
            if (is_files) {
                capture.has_files = true;
                file_preview = FilesPreview(handle);
            }
            capture.data.push_back(std::move(data));
        }

        if (capture.data.empty()) {
            return std::nullopt;
        }
        capture.fingerprint = HashCapture(capture.data);
        capture.preview = !text.empty() ? text : file_preview;
        if (capture.preview.empty() && capture.has_image) {
            capture.preview = L"[图片]";
        }
        if (capture.preview.empty() && capture.has_files) {
            capture.preview = L"[文件]";
        }
        capture.title = MakeTitle(capture.preview, m_settings.show_special_symbols);
        if (capture.title.empty()) {
            capture.title = capture.has_image ? L"[图片]" : (capture.has_files ? L"[文件]" : L"[剪贴板项目]");
        }
        return capture;
    }

    bool ReadClipboardAndSave() {
        for (int attempt = 0; attempt < 5; ++attempt) {
            if (!::OpenClipboard(m_hWnd)) {
                Sleep(5);
                continue;
            }
            const auto capture = CaptureClipboard();
            CloseClipboard();
            if (!capture) {
                return false;
            }
            m_database.SaveClipboard(*capture, m_settings.history_size);
            m_lastCopyText = PreviewText(capture->preview);
            UpdateTrayTooltip();
            return true;
        }
        return false;
    }

    bool SetClipboardItem(const ClipboardItem &item, bool remove_formatting) {
        if (item.data.empty()) {
            return false;
        }
        if (!::OpenClipboard(m_hWnd)) {
            return false;
        }
        bool success = EmptyClipboard() != FALSE;
        bool has_text = false;
        const bool has_plain_text = std::any_of(
            item.data.begin(),
            item.data.end(),
            [](const ClipboardFormatData &data) {
                return data.format == CF_UNICODETEXT || data.format == CF_TEXT ||
                    data.name == L"CF_UNICODETEXT" || data.name == L"CF_TEXT";
            }
        );
        if (success) {
            for (const ClipboardFormatData &data : item.data) {
                const bool is_text = data.format == CF_UNICODETEXT || data.format == CF_TEXT ||
                    data.name == L"CF_UNICODETEXT" || data.name == L"CF_TEXT";
                const bool is_files = data.format == CF_HDROP || data.name == L"CF_HDROP";
                if (remove_formatting && has_plain_text && !is_text && !is_files) {
                    continue;
                }
                UINT format = data.format;
                if (format >= 0xC000 || format == 0) {
                    format = RegisterClipboardFormatW(data.name.c_str());
                }
                if (format == 0 || data.bytes.empty()) {
                    continue;
                }
                HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, data.bytes.size());
                if (handle == nullptr) {
                    success = false;
                    break;
                }
                void *destination = GlobalLock(handle);
                if (destination == nullptr) {
                    GlobalFree(handle);
                    success = false;
                    break;
                }
                std::memcpy(destination, data.bytes.data(), data.bytes.size());
                GlobalUnlock(handle);
                if (SetClipboardData(format, handle) == nullptr) {
                    GlobalFree(handle);
                    success = false;
                    break;
                }
                has_text = has_text || is_text;
            }
        }
        CloseClipboard();
        if (success && has_plain_text && !has_text) {
            return false;
        }
        return success;
    }

    void RestoreTargetFocusAndPaste(HWND target, HWND target_focus, bool paste) {
        if (!::IsWindow(target)) {
            return;
        }
        if (::IsIconic(target)) {
            ::ShowWindow(target, SW_RESTORE);
        }
        const DWORD current_thread = GetCurrentThreadId();
        const DWORD target_thread = GetWindowThreadProcessId(target, nullptr);
        const bool attached = target_thread != 0 && target_thread != current_thread &&
            AttachThreadInput(current_thread, target_thread, TRUE) != FALSE;
        SetForegroundWindow(target);
        if (attached && ::IsWindow(target_focus) && GetAncestor(target_focus, GA_ROOT) == target) {
            ::SetFocus(target_focus);
        }
        if (attached) {
            AttachThreadInput(current_thread, target_thread, FALSE);
        }
        SetForegroundWindow(target);
        if (!paste) {
            return;
        }
        const HWND active_window = GetForegroundWindow();
        if (active_window != target && GetAncestor(active_window, GA_ROOT) != target) {
            return;
        }
        INPUT inputs[4]{};
        inputs[0].type = INPUT_KEYBOARD;
        inputs[0].ki.wVk = VK_CONTROL;
        inputs[1].type = INPUT_KEYBOARD;
        inputs[1].ki.wVk = L'V';
        inputs[2].type = INPUT_KEYBOARD;
        inputs[2].ki.wVk = L'V';
        inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
        inputs[3].type = INPUT_KEYBOARD;
        inputs[3].ki.wVk = VK_CONTROL;
        inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
    }

    std::wstring DisplayText(const ClipboardItem &item) const {
        if (!item.title.empty()) {
            return item.title;
        }
        if (!item.content.empty()) {
            return PreviewText(item.content);
        }
        if (item.has_image) {
            return L"[图片]";
        }
        if (item.has_files) {
            return L"[文件]";
        }
        return L"[剪贴板项目]";
    }

    int HistoryItemAtPoint(HWND window, POINT point) const {
        if (window == nullptr || window != m_historyList || !::IsWindow(window)) {
            return -1;
        }
        const LRESULT hit = SendMessageW(
            window,
            LB_ITEMFROMPOINT,
            0,
            MAKELPARAM(point.x, point.y)
        );
        if (HIWORD(hit) != 0) {
            return -1;
        }
        const int index = static_cast<int>(LOWORD(hit));
        if (index < 0 || static_cast<size_t>(index) >= m_items.size()) {
            return -1;
        }
        return index;
    }

    void InvalidateHistoryItem(int index) {
        if (m_historyList == nullptr || index < 0) {
            return;
        }
        RECT item_rect{};
        if (SendMessageW(m_historyList, LB_GETITEMRECT, index, reinterpret_cast<LPARAM>(&item_rect)) != LB_ERR) {
            ::InvalidateRect(m_historyList, &item_rect, TRUE);
        }
    }

    void BeginHistoryMouseTracking() {
        if (m_historyList == nullptr || m_historyMouseTracking) {
            return;
        }
        TRACKMOUSEEVENT tracking{};
        tracking.cbSize = sizeof(tracking);
        tracking.dwFlags = TME_LEAVE;
        tracking.hwndTrack = m_historyList;
        if (::TrackMouseEvent(&tracking) != FALSE) {
            m_historyMouseTracking = true;
        }
    }

    void OnHistoryMouseMove(HWND window, POINT point) {
        if (!m_popupVisible || window != m_historyList) {
            return;
        }
        // Hovering a row must not make the search field disappear or leave
        // the list covering it. Re-assert the visibility required by the
        // current search setting before processing the row hit-test.
        const bool search_should_be_visible = m_settings.show_search &&
            (m_settings.search_visibility == SearchVisibility::Always || !m_searchQuery.empty());
        if (search_should_be_visible && !::IsWindowVisible(m_search)) {
            SetHistorySearchVisible(true);
        }
        BeginHistoryMouseTracking();

        const int index = HistoryItemAtPoint(window, point);
        if (index < 0) {
            ClearHistoryHover();
            return;
        }

        const sqlite3_int64 item_id = m_items[static_cast<size_t>(index)].id;
        if (index == m_hoveredItemIndex && item_id == m_hoveredItemId) {
            const LRESULT selected = SendMessageW(m_historyList, LB_GETCURSEL, 0, 0);
            if (selected != index) {
                m_mouseSelectionUpdate = true;
                SendMessageW(m_historyList, LB_SETCURSEL, index, 0);
                m_mouseSelectionUpdate = false;
                InvalidateHistoryItem(selected == LB_ERR ? -1 : static_cast<int>(selected));
                InvalidateHistoryItem(index);
            }
            if (!m_previewWindow.IsVisible() && m_previewCandidateId == 0) {
                SchedulePreviewForHoveredItem();
            }
            return;
        }

        const int previous_index = m_hoveredItemIndex;
        m_hoveredItemIndex = index;
        m_hoveredItemId = item_id;

        m_mouseSelectionUpdate = true;
        SendMessageW(m_historyList, LB_SETCURSEL, index, 0);
        m_mouseSelectionUpdate = false;

        InvalidateHistoryItem(previous_index);
        InvalidateHistoryItem(index);

        if (m_previewWindow.IsVisible()) {
            ShowPreviewForItem(item_id);
        } else {
            SchedulePreviewForHoveredItem();
        }
    }

    void OnHistoryMouseLeave() {
        m_historyMouseTracking = false;
        ClearHistoryHover();
    }

    void UpdateHistoryHoverFromCursor() {
        if (!m_popupVisible || m_historyList == nullptr ||
            !::IsWindowVisible(m_historyList)) {
            return;
        }

        POINT point{};
        if (::GetCursorPos(&point) == FALSE ||
            ::ScreenToClient(m_historyList, &point) == FALSE) {
            m_historyMouseTracking = false;
            ClearHistoryHover();
            return;
        }

        RECT client{};
        ::GetClientRect(m_historyList, &client);
        if (!::PtInRect(&client, point)) {
            m_historyMouseTracking = false;
            ClearHistoryHover();
            return;
        }
        OnHistoryMouseMove(m_historyList, point);
    }

    void ClearHistoryHover() {
        const int previous_index = m_hoveredItemIndex;
        m_hoveredItemIndex = -1;
        m_hoveredItemId = 0;
        HidePreview();
        InvalidateHistoryItem(previous_index);
    }

    void RefreshHistory(std::wstring_view query) {
        try {
            HidePreview();
            m_hoveredItemIndex = -1;
            m_hoveredItemId = 0;
            m_searchQuery = std::wstring(query);
            m_items = m_database.SearchHistory(
                query,
                static_cast<int>(m_settings.search_mode),
                m_settings.sort_by,
                m_settings.pin_to == PinPosition::Bottom
            );
            m_loadingList = true;
            SendMessageW(m_historyList, LB_RESETCONTENT, 0, 0);
            for (const ClipboardItem &item : m_items) {
                const std::wstring display = DisplayText(item);
                SendMessageW(m_historyList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
            }
            if (!m_items.empty()) {
                SendMessageW(m_historyList, LB_SETCURSEL, 0, 0);
            }
            m_loadingList = false;
            ApplyHistoryVisibility();
            if (m_popupVisible) {
                UpdateHistoryHoverFromCursor();
            }
        } catch (const std::exception &error) {
            m_loadingList = false;
            OutputDebugStringA(error.what());
            OutputDebugStringA("\n");
        }
    }

    void ScheduleSearch() {
        KillTimer(kSearchTimerId);
        ::SetTimer(m_hWnd, kSearchTimerId, kSearchDebounceMilliseconds, nullptr);
    }

    void SchedulePreviewForHoveredItem() {
        KillTimer(kPreviewTimerId);
        m_previewCandidateId = 0;
        if (!m_popupVisible || !m_settings.open_preview_automatically ||
            m_hoveredItemId == 0 || m_previewWindow.IsVisible()) {
            return;
        }
        m_previewCandidateId = m_hoveredItemId;
        ::SetTimer(m_hWnd, kPreviewTimerId, static_cast<UINT>(m_settings.preview_delay), nullptr);
    }

    void ShowPreviewForItem(sqlite3_int64 item_id) {
        if (item_id <= 0) {
            HidePreview();
            return;
        }
        KillTimer(kPreviewTimerId);
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

    void ShowPreviewForHoveredItem() {
        if (!m_popupVisible || m_hoveredItemId == 0 ||
            m_previewCandidateId != m_hoveredItemId) {
            HidePreview();
            return;
        }

        POINT point{};
        if (::GetCursorPos(&point) == FALSE ||
            ::ScreenToClient(m_historyList, &point) == FALSE) {
            ClearHistoryHover();
            return;
        }
        const int index = HistoryItemAtPoint(m_historyList, point);
        if (index < 0 || m_items[static_cast<size_t>(index)].id != m_hoveredItemId) {
            ClearHistoryHover();
            return;
        }
        ShowPreviewForItem(m_hoveredItemId);
    }

    void ShowPreviewForSelection() {
        const LRESULT selected = SendMessageW(m_historyList, LB_GETCURSEL, 0, 0);
        if (selected == LB_ERR || static_cast<size_t>(selected) >= m_items.size()) {
            HidePreview();
            return;
        }
        ShowPreviewForItem(m_items[static_cast<size_t>(selected)].id);
    }

    void HidePreview() {
        KillTimer(kPreviewTimerId);
        m_previewCandidateId = 0;
        m_previewItemId = 0;
        m_previewWindow.Hide();
    }

    std::vector<std::pair<size_t, size_t>> HighlightRanges(std::wstring_view text) const {
        std::vector<std::pair<size_t, size_t>> ranges;
        if (m_searchQuery.empty()) {
            return ranges;
        }
        const auto add_exact = [&]() {
            const std::wstring lower_text = Lower(text);
            const std::wstring lower_query = Lower(m_searchQuery);
            if (lower_query.empty()) {
                return;
            }
            size_t position = 0;
            while ((position = lower_text.find(lower_query, position)) != std::wstring::npos) {
                ranges.emplace_back(position, position + lower_query.size());
                position += lower_query.size();
            }
        };
        const auto add_regex = [&]() {
            try {
                const std::wregex expression{m_searchQuery};
                const std::wstring value(text);
                for (std::wsregex_iterator it(value.begin(), value.end(), expression), end; it != end; ++it) {
                    const auto match = *it;
                    ranges.emplace_back(
                        static_cast<size_t>(match.position()),
                        static_cast<size_t>(match.position() + match.length())
                    );
                }
            } catch (const std::regex_error &) {
            }
        };
        const auto add_fuzzy = [&]() {
            const std::wstring lower_text = Lower(text);
            const std::wstring lower_query = Lower(m_searchQuery);
            size_t text_position = 0;
            size_t start = std::wstring::npos;
            size_t last = std::wstring::npos;
            for (const wchar_t expected : lower_query) {
                const size_t found = lower_text.find(expected, text_position);
                if (found == std::wstring::npos) {
                    ranges.clear();
                    return;
                }
                if (start == std::wstring::npos) {
                    start = found;
                }
                last = found;
                text_position = found + 1;
            }
            if (start != std::wstring::npos) {
                ranges.emplace_back(start, last + 1);
            }
        };

        switch (m_settings.search_mode) {
        case SearchMode::Regexp:
            add_regex();
            break;
        case SearchMode::Fuzzy:
            add_fuzzy();
            break;
        case SearchMode::Mixed:
            add_exact();
            if (ranges.empty()) {
                add_regex();
            }
            if (ranges.empty()) {
                add_fuzzy();
            }
            break;
        case SearchMode::Exact:
        default:
            add_exact();
            break;
        }
        return ranges;
    }

    HFONT FontForHighlight(HighlightMatch match) const {
        switch (match) {
        case HighlightMatch::Bold:
            return m_boldFont;
        case HighlightMatch::Italic:
            return m_italicFont;
        case HighlightMatch::Underline:
            return m_underlineFont;
        case HighlightMatch::Color:
        default:
            return m_normalFont;
        }
    }

    void DrawTextWithHighlights(HDC dc, RECT rect, std::wstring_view text, bool selected) {
        const auto ranges = HighlightRanges(text);
        std::vector<std::pair<size_t, size_t>> segments;
        size_t position = 0;
        for (const auto &[start, end] : ranges) {
            if (start > position) {
                segments.emplace_back(position, start);
            }
            segments.emplace_back(start, std::max(start, end));
            position = std::max(position, end);
        }
        if (position < text.size()) {
            segments.emplace_back(position, text.size());
        }
        if (segments.empty()) {
            segments.emplace_back(0, text.size());
        }

        const int old_bk_mode = SetBkMode(dc, TRANSPARENT);
        const COLORREF old_text = GetTextColor(dc);
        int x = rect.left;
        for (const auto &[start, end] : segments) {
            if (end <= start) {
                continue;
            }
            const bool highlighted = std::any_of(
                ranges.begin(),
                ranges.end(),
                [start, end](const auto &range) { return start >= range.first && end <= range.second; }
            );
            const std::wstring part(text.substr(start, end - start));
            SIZE size{};
            HFONT font = highlighted ? FontForHighlight(m_settings.highlight_match) : m_normalFont;
            HGDIOBJ old_font = SelectObject(dc, font != nullptr ? font : GetStockObject(DEFAULT_GUI_FONT));
            if (highlighted && m_settings.highlight_match == HighlightMatch::Color) {
                SetTextColor(dc, RGB(0, 70, 160));
                SetBkMode(dc, OPAQUE);
                SetBkColor(dc, RGB(255, 239, 160));
            } else {
                SetTextColor(dc, selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : old_text);
                SetBkMode(dc, TRANSPARENT);
            }
            GetTextExtentPoint32W(dc, part.c_str(), static_cast<int>(part.size()), &size);
            TextOutW(dc, x, rect.top, part.c_str(), static_cast<int>(part.size()));
            x += size.cx;
            SelectObject(dc, old_font);
            if (x >= rect.right) {
                break;
            }
        }
        SetTextColor(dc, old_text);
        SetBkMode(dc, old_bk_mode);
    }

    HICON IconForApplication(std::wstring_view application) {
        if (application.empty()) {
            return nullptr;
        }
        const std::wstring key = NormalizePath(std::wstring(application));
        if (const auto found = m_iconCache.find(key); found != m_iconCache.end()) {
            return found->second;
        }
        SHFILEINFOW info{};
        if (SHGetFileInfoW(
            application.data(),
            0,
            &info,
            sizeof(info),
            SHGFI_ICON | SHGFI_SMALLICON
        ) == 0) {
            return nullptr;
        }
        if (m_iconCache.size() >= 32) {
            auto first = m_iconCache.begin();
            DestroyIcon(first->second);
            m_iconCache.erase(first);
        }
        m_iconCache.emplace(key, info.hIcon);
        return info.hIcon;
    }

    void DrawHistoryItem(DRAWITEMSTRUCT *draw) {
        if (draw == nullptr || draw->itemID == static_cast<UINT>(-1) ||
            static_cast<size_t>(draw->itemID) >= m_items.size()) {
            return;
        }
        const ClipboardItem &item = m_items[draw->itemID];
        const bool selected = (draw->itemState & ODS_SELECTED) != 0;
        FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW));
        RECT text_rect = draw->rcItem;
        text_rect.left += 10;
        text_rect.top += 6;
        text_rect.bottom -= 4;

        if (m_settings.show_application_icons) {
            if (const HICON icon = IconForApplication(item.application)) {
                DrawIconEx(draw->hDC, text_rect.left, text_rect.top, icon, 16, 16, 0, nullptr, DI_NORMAL);
                text_rect.left += 22;
            }
        }
        if (item.has_image) {
            RECT image_rect{text_rect.left, text_rect.top, text_rect.left + 24, text_rect.top + 24};
            HBRUSH brush = CreateSolidBrush(RGB(225, 230, 235));
            FillRect(draw->hDC, &image_rect, brush);
            DeleteObject(brush);
            FrameRect(draw->hDC, &image_rect, GetSysColorBrush(COLOR_GRAYTEXT));
            text_rect.left += 32;
        } else if (item.has_files) {
            RECT file_rect{text_rect.left, text_rect.top, text_rect.left + 24, text_rect.top + 20};
            HBRUSH brush = CreateSolidBrush(RGB(250, 220, 130));
            FillRect(draw->hDC, &file_rect, brush);
            DeleteObject(brush);
            text_rect.left += 32;
        }

        const std::wstring text = DisplayText(item);
        DrawTextWithHighlights(draw->hDC, text_rect, text, selected);
        if (m_settings.show_hex_color_swatch) {
            const size_t hash = text.find(L'#');
            if (hash != std::wstring::npos && hash + 7 <= text.size()) {
                const std::wstring hex = text.substr(hash + 1, 6);
                wchar_t *end = nullptr;
                const unsigned long value = wcstoul(hex.c_str(), &end, 16);
                if (end == hex.c_str() + 6) {
                    const COLORREF color = RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
                    RECT swatch{draw->rcItem.right - 28, draw->rcItem.top + 7, draw->rcItem.right - 10, draw->rcItem.top + 25};
                    HBRUSH brush = CreateSolidBrush(color);
                    FillRect(draw->hDC, &swatch, brush);
                    DeleteObject(brush);
                    FrameRect(draw->hDC, &swatch, GetSysColorBrush(COLOR_GRAYTEXT));
                }
            }
        }
    }

    void CreateFonts() {
        LOGFONTW log_font{};
        const HFONT stock = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        GetObjectW(stock, sizeof(log_font), &log_font);
        m_normalFont = stock;
        log_font.lfWeight = FW_BOLD;
        m_boldFont = CreateFontIndirectW(&log_font);
        log_font.lfWeight = FW_NORMAL;
        log_font.lfItalic = TRUE;
        m_italicFont = CreateFontIndirectW(&log_font);
        log_font.lfItalic = FALSE;
        log_font.lfUnderline = TRUE;
        m_underlineFont = CreateFontIndirectW(&log_font);
    }

    void DestroyFonts() {
        for (HFONT font : {m_boldFont, m_italicFont, m_underlineFont}) {
            if (font != nullptr) {
                DeleteObject(font);
            }
        }
        m_boldFont = nullptr;
        m_italicFont = nullptr;
        m_underlineFont = nullptr;
        for (auto &[path, icon] : m_iconCache) {
            (void)path;
            if (icon != nullptr) {
                DestroyIcon(icon);
            }
        }
        m_iconCache.clear();
    }

    void HideMainWindow() {
        if (!m_popupVisible) {
            return;
        }
        RECT rect{};
        if (::GetWindowRect(m_hWnd, &rect)) {
            m_settings.popup_x = rect.left;
            m_settings.popup_y = rect.top;
            try {
                m_database.SetSetting(L"appearance.windowX", std::to_wstring(m_settings.popup_x));
                m_database.SetSetting(L"appearance.windowY", std::to_wstring(m_settings.popup_y));
            } catch (...) {
            }
        }
        m_popupVisible = false;
        HidePreview();
        ShowWindow(SW_HIDE);
    }

    bool HasHistoryItems() const {
        return !m_items.empty();
    }

    void NavigateHistoryFromSearch(bool forward) {
        if (!m_popupVisible || m_historyList == nullptr || m_items.empty()) {
            return;
        }

        const int current = SelectedHistoryIndex();
        int target = current;
        if (current < 0) {
            target = forward ? 0 : static_cast<int>(m_items.size()) - 1;
        } else {
            target = current + (forward ? 1 : -1);
            if (target < 0 || static_cast<size_t>(target) >= m_items.size()) {
                ::SetFocus(m_search);
                return;
            }
        }

        const int previous_hover = m_hoveredItemIndex;
        m_mouseSelectionUpdate = true;
        SendMessageW(m_historyList, LB_SETCURSEL, target, 0);
        m_mouseSelectionUpdate = false;

        // Keyboard navigation takes precedence until the pointer moves again.
        // Keeping the edit focused lets its normal left/right caret behavior
        // continue to work without losing the list selection.
        m_hoveredItemIndex = -1;
        m_hoveredItemId = 0;
        HidePreview();
        InvalidateHistoryItem(previous_hover);
        ::SetFocus(m_search);
    }

    int SelectedHistoryIndex() const {
        const LRESULT selected = SendMessageW(m_historyList, LB_GETCURSEL, 0, 0);
        if (selected == LB_ERR || static_cast<size_t>(selected) >= m_items.size()) {
            return -1;
        }
        return static_cast<int>(selected);
    }

    std::wstring NextPinKey() const {
        constexpr wchar_t keys[] = L"bcdefghijklmnoprstuxy";
        for (const wchar_t key : keys) {
            bool used = false;
            for (const ClipboardItem &item : m_items) {
                if (item.pinned && item.pin.size() == 1 && item.pin[0] == key) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                return std::wstring(1, key);
            }
        }
        return L"p";
    }

    void ToggleSelectedPin() {
        const int index = SelectedHistoryIndex();
        if (index < 0) {
            return;
        }
        ClipboardItem &item = m_items[static_cast<size_t>(index)];
        try {
            if (item.pinned) {
                m_database.TogglePin(item.id, {}, false);
            } else {
                m_database.TogglePin(item.id, NextPinKey(), true);
            }
            RefreshHistory(m_searchQuery);
        } catch (const std::exception &error) {
            OutputDebugStringA(error.what());
            OutputDebugStringA("\n");
        }
    }

    void DeleteSelectedItem() {
        const int index = SelectedHistoryIndex();
        if (index < 0) {
            return;
        }
        try {
            m_database.DeleteItem(m_items[static_cast<size_t>(index)].id);
            RefreshHistory(m_searchQuery);
        } catch (const std::exception &error) {
            OutputDebugStringA(error.what());
            OutputDebugStringA("\n");
        }
    }

    bool HandlePopupShortcut(WPARAM key) {
        if (IsHotKeyPressed(m_settings.pin_hotkey, key)) {
            ToggleSelectedPin();
            return true;
        }
        if (IsHotKeyPressed(m_settings.delete_hotkey, key)) {
            DeleteSelectedItem();
            return true;
        }
        if (IsHotKeyPressed(m_settings.preview_hotkey, key)) {
            if (m_previewWindow.IsVisible()) {
                HidePreview();
            } else {
                ShowPreviewForSelection();
            }
            return true;
        }
        return false;
    }

    void PasteItem(int index) {
        if (m_loadingList || m_pasting || index < 0 || static_cast<size_t>(index) >= m_items.size()) {
            return;
        }
        const HWND target = m_targetWindow;
        const HWND target_focus = m_targetFocusWindow;
        const sqlite3_int64 id = m_items[static_cast<size_t>(index)].id;
        try {
            const auto item = m_database.GetItem(id, true);
            if (!item || !SetClipboardItem(*item, m_settings.remove_formatting_by_default)) {
                return;
            }
            m_pasting = true;
            // EmptyClipboard/SetClipboardData posts WM_CLIPBOARDUPDATE.  Do
            // not immediately record the item that the user just selected.
            m_skipNextClipboardEvent = true;
            HideMainWindow();
            RestoreTargetFocusAndPaste(target, target_focus, m_settings.paste_by_default);
            m_pasting = false;
        } catch (const std::exception &error) {
            m_pasting = false;
            OutputDebugStringA(error.what());
            OutputDebugStringA("\n");
        }
    }

    void PasteSelectedItem() {
        const int selected = SelectedHistoryIndex();
        if (selected >= 0) {
            PasteItem(selected);
        }
    }

    void UpdateTrayTooltip() {
        std::wstring tooltip = L"剪贴板历史";
        if (m_settings.show_recent_copy_in_menu_bar && !m_lastCopyText.empty()) {
            tooltip += L" - " + m_lastCopyText;
        }
        lstrcpynW(m_notifyIcon.szTip, tooltip.c_str(), ARRAYSIZE(m_notifyIcon.szTip));
        if (m_trayIconAdded) {
            m_notifyIcon.uFlags = NIF_TIP | NIF_ICON | NIF_MESSAGE;
            Shell_NotifyIconW(NIM_MODIFY, &m_notifyIcon);
        }
    }

    void UpdateTrayIcon() {
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

    void ShowTrayMenu() {
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

    void RemoveTrayIcon() {
        if (m_trayIconAdded) {
            Shell_NotifyIconW(NIM_DELETE, &m_notifyIcon);
            m_trayIconAdded = false;
        }
    }

    void ClearHistory() {
        try {
            m_database.DeleteUnpinned();
            if (m_settings.clear_system_clipboard && ::OpenClipboard(m_hWnd)) {
                EmptyClipboard();
                CloseClipboard();
            }
            RefreshHistory(L"");
        } catch (const std::exception &error) {
            MessageBoxA(m_hWnd, error.what(), "Unable to clear history", MB_OK | MB_ICONERROR);
        }
    }

    void OpenSettings() {
        if (m_settingsWindow == nullptr) {
            m_settingsWindow = std::make_unique<SettingsWindow>(m_database, m_hWnd);
        }
        if (!m_settingsWindow->CreateOrShow()) {
            ::MessageBoxW(m_hWnd, L"无法打开设置窗口。", L"剪贴板", MB_OK | MB_ICONERROR);
        }
    }

    void ExitApplication() {
        m_exiting = true;
        if (m_settings.clear_on_quit) {
            try {
                m_database.DeleteUnpinned();
            } catch (...) {
            }
        }
        if (m_settings.clear_system_clipboard && ::OpenClipboard(m_hWnd)) {
            EmptyClipboard();
            CloseClipboard();
        }
        RemoveTrayIcon();
        if (m_settingsWindow != nullptr) {
            m_settingsWindow->DestroyForOwner();
        }
        DestroyWindow();
    }

    void ApplySettings() {
        const AppSettings previous = m_settings;
        m_settings = AppSettings::Load(m_database);
        ReloadIgnoreLists();
        if (!SameHotKey(previous.open_hotkey, m_settings.open_hotkey) ||
            previous.show_in_status_bar != m_settings.show_in_status_bar) {
            if (m_hotkeyRegistered) {
                UnregisterHotKey(m_hWnd, kHotkeyId);
                m_hotkeyRegistered = false;
            }
            m_hotkeyRegistered = RegisterHotKey(
                m_hWnd,
                kHotkeyId,
                m_settings.open_hotkey.modifiers | MOD_NOREPEAT,
                m_settings.open_hotkey.virtual_key
            ) == TRUE;
        }
        UpdateTrayIcon();
        if (!m_settings.show_search) {
        ::SetWindowTextW(m_search, L"");
            m_searchQuery.clear();
        }
        RefreshHistory(m_searchQuery);
        ApplyHistoryVisibility();
    }

    void ScheduleSearchFromCurrentEdit() {
        m_searchQuery = ReadWindowText(m_search);
        ScheduleSearch();
    }

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        CreateFonts();
        if (!BindControls() || !m_previewWindow.Initialize(m_hWnd)) {
            handled = FALSE;
            return FALSE;
        }
        ReloadIgnoreLists();
        m_clipboardListenerAdded = AddClipboardFormatListener(m_hWnd) == TRUE;
        m_hotkeyRegistered = RegisterHotKey(
            m_hWnd,
            kHotkeyId,
            m_settings.open_hotkey.modifiers | MOD_NOREPEAT,
            m_settings.open_hotkey.virtual_key
        ) == TRUE;
        RefreshHistory(L"");
        ApplyHistoryVisibility();
        return TRUE;
    }

    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        LayoutHistoryControls();
        if (m_previewWindow.IsVisible()) {
            PositionPreviewWindow();
        }
        return 0;
    }

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL &) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(&paint);
        RECT client{};
        GetClientRect(&client);
        FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));
        EndPaint(&paint);
        return 0;
    }

    LRESULT OnEraseBackground(UINT, WPARAM, LPARAM, BOOL &) {
        return 1;
    }

    LRESULT OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
        auto *measure = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
        if (measure == nullptr || measure->CtlID != kHistoryListControlId) {
            handled = FALSE;
            return 0;
        }
        handled = TRUE;
        measure->itemHeight = 32;
        if (measure->itemID < m_items.size() && m_items[measure->itemID].has_image) {
            measure->itemHeight = static_cast<UINT>(std::clamp(m_settings.image_max_height + 12, 32, 212));
        }
        return 0;
    }

    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
        auto *draw = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
        if (draw == nullptr || draw->CtlID != kHistoryListControlId) {
            handled = FALSE;
            return 0;
        }
        handled = TRUE;
        DrawHistoryItem(draw);
        return 0;
    }

    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
        if (LOWORD(wParam) == WA_INACTIVE) {
            const HWND activating_window = reinterpret_cast<HWND>(lParam);
            if (activating_window != nullptr && !m_trayMenuShowing && !IsOurWindow(activating_window)) {
                CaptureTargetWindow(activating_window);
            }
            if (m_popupVisible && !m_exiting && !m_trayMenuShowing && !IsOurWindow(activating_window)) {
                HideMainWindow();
            }
        }
        return 0;
    }

    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &) {
        if (m_exiting) {
            DestroyWindow();
        } else {
            HideMainWindow();
        }
        return 0;
    }

    LRESULT OnCommand(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        const UINT command = LOWORD(wParam);
        const UINT notification = HIWORD(wParam);
        if (command == kTrayCommandShow && notification == 0) {
            handled = TRUE;
            ShowMainWindow();
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
            return 0;
        }
        if (command == kTrayCommandExit && notification == 0) {
            handled = TRUE;
            ExitApplication();
            return 0;
        }
        if (command == kSearchControlId && notification == EN_CHANGE) {
            handled = TRUE;
            ScheduleSearchFromCurrentEdit();
            return 0;
        }
        if (command == kHistoryListControlId && notification == LBN_SELCHANGE) {
            handled = TRUE;
            if (!m_loadingList && !m_mouseSelectionUpdate) {
                UpdateHistoryHoverFromCursor();
            }
            return 0;
        }
        handled = FALSE;
        return 0;
    }

    LRESULT OnTimer(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        if (wParam == kSearchTimerId) {
            handled = TRUE;
            KillTimer(kSearchTimerId);
            RefreshHistory(ReadWindowText(m_search));
            return 0;
        }
        if (wParam == kPreviewTimerId) {
            handled = TRUE;
            KillTimer(kPreviewTimerId);
            ShowPreviewForHoveredItem();
            return 0;
        }
        handled = FALSE;
        return 0;
    }

    LRESULT OnClipboardUpdate(UINT, WPARAM, LPARAM, BOOL &) {
        if (m_pasting) {
            return 0;
        }
        if (m_skipNextClipboardEvent) {
            m_skipNextClipboardEvent = false;
            return 0;
        }
        try {
            if (m_settings.ignore_events) {
                if (m_settings.ignore_only_next_event) {
                    m_settings.ignore_events = false;
                    m_settings.ignore_only_next_event = false;
                    m_settings.Save(m_database);
                }
                return 0;
            }
            if (ReadClipboardAndSave() && m_popupVisible) {
                RefreshHistory(ReadWindowText(m_search));
            }
        } catch (const std::exception &error) {
            OutputDebugStringA(error.what());
            OutputDebugStringA("\n");
        }
        return 0;
    }

    LRESULT OnHotKey(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        if (wParam != kHotkeyId) {
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

    LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        if (HandlePopupShortcut(wParam)) {
            handled = TRUE;
            return 0;
        }
        if (wParam == L'F' && (::GetKeyState(VK_CONTROL) & 0x8000) != 0 && m_settings.show_search) {
            SetHistorySearchVisible(true);
            ::SetFocus(m_search);
            handled = TRUE;
            return 0;
        }
        if (wParam == VK_ESCAPE) {
            HideMainWindow();
            handled = TRUE;
            return 0;
        }
        handled = FALSE;
        return 0;
    }

    LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        if (m_settings.show_search && m_settings.search_visibility == SearchVisibility::DuringSearch &&
            wParam >= 0x20 && wParam != 0x7F) {
            SetHistorySearchVisible(true);
            ::SetFocus(m_search);
            const wchar_t character = static_cast<wchar_t>(wParam);
            const wchar_t text[] = {character, L'\0'};
            SendMessageW(m_search, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text));
            ScheduleSearch();
            handled = TRUE;
            return 0;
        }
        handled = FALSE;
        return 0;
    }

    LRESULT OnTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
        if (wParam != kTrayIconId) {
            return 0;
        }
        switch (static_cast<UINT>(lParam)) {
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
            ShowMainWindow();
            break;
        case WM_RBUTTONUP:
        case WM_CONTEXTMENU:
            CaptureTargetWindow();
            ShowTrayMenu();
            break;
        default:
            break;
        }
        return 0;
    }

    LRESULT OnSettingsChanged(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        ApplySettings();
        return 0;
    }

    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &) {
        m_popupVisible = false;
        KillTimer(kSearchTimerId);
        KillTimer(kPreviewTimerId);
        if (m_hotkeyRegistered) {
            UnregisterHotKey(m_hWnd, kHotkeyId);
            m_hotkeyRegistered = false;
        }
        if (m_clipboardListenerAdded) {
            RemoveClipboardFormatListener(m_hWnd);
            m_clipboardListenerAdded = false;
        }
        if (m_previewWindow.Window() != nullptr && ::IsWindow(m_previewWindow.Window())) {
            m_previewWindow.DestroyWindow();
        }
        RestoreControlSubclasses();
        RemoveTrayIcon();
        DestroyFonts();
        PostQuitMessage(0);
        return 0;
    }

    Database &m_database;
    AppSettings m_settings;
    HWND m_search = nullptr;
    HWND m_historyList = nullptr;
    PreviewWindow m_previewWindow;
    WNDPROC m_originalSearchProc = nullptr;
    WNDPROC m_originalHistoryListProc = nullptr;
    std::vector<ClipboardItem> m_items;
    std::vector<std::wstring> m_ignoredApps;
    std::vector<std::wstring> m_ignoredFormats;
    std::vector<std::wstring> m_ignoredRegexps;
    std::unordered_map<std::wstring, HICON> m_iconCache;
    std::unique_ptr<SettingsWindow> m_settingsWindow;
    NOTIFYICONDATAW m_notifyIcon{};
    HWND m_targetWindow = nullptr;
    HWND m_targetFocusWindow = nullptr;
    RECT m_targetCaretRect{};
    HFONT m_normalFont = nullptr;
    HFONT m_boldFont = nullptr;
    HFONT m_italicFont = nullptr;
    HFONT m_underlineFont = nullptr;
    std::wstring m_searchQuery;
    std::wstring m_lastCopyText;
    int m_searchHeight = 0;
    int m_hoveredItemIndex = -1;
    sqlite3_int64 m_hoveredItemId = 0;
    sqlite3_int64 m_previewCandidateId = 0;
    sqlite3_int64 m_previewItemId = 0;
    bool m_trayIconAdded = false;
    bool m_clipboardListenerAdded = false;
    bool m_hotkeyRegistered = false;
    bool m_popupVisible = false;
    bool m_loadingList = false;
    bool m_historyMouseTracking = false;
    bool m_mouseSelectionUpdate = false;
    bool m_pasting = false;
    bool m_skipNextClipboardEvent = false;
    bool m_hasTargetCaretRect = false;
    bool m_exiting = false;
    bool m_trayMenuShowing = false;
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
        return 1;
    }
    if (FAILED(_Module.Init(nullptr, instance))) {
        CoUninitialize();
        return 1;
    }
    // The application uses only the Tab and HotKey common controls.  The
    // standard controls (buttons, edits, list boxes, etc.) are provided by
    // USER32 and do not need to be included here.  In particular, asking
    // older comctl32 builds to initialize ICC_STANDARD_CLASSES can make
    // InitCommonControlsEx return FALSE and trip WTL's debug assertion.
    INITCOMMONCONTROLSEX common_controls{
        sizeof(INITCOMMONCONTROLSEX),
        ICC_TAB_CLASSES | ICC_HOTKEY_CLASS
    };
    if (!::InitCommonControlsEx(&common_controls)) {
        const DWORD error = GetLastError();
        _Module.Term();
        CoUninitialize();
        const std::wstring message =
            L"Unable to initialize Windows common controls. Error code: " +
            std::to_wstring(error);
        MessageBoxW(nullptr, message.c_str(), L"Clipboard error", MB_OK | MB_ICONERROR);
        return 1;
    }

    try {
        Database database(GetDatabasePath());
        MainWindow window(database);
        if (!window.Create(nullptr)) {
            _Module.Term();
            CoUninitialize();
            return 1;
        }

        if (!window.AddTrayIcon()) {
            window.ShowMainWindow();
        }

        MSG message{};
        while (GetMessageW(&message, nullptr, 0, 0) > 0) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        _Module.Term();
        CoUninitialize();
        return static_cast<int>(message.wParam);
    } catch (const std::exception &error) {
        MessageBoxA(nullptr, error.what(), "Clipboard error", MB_OK | MB_ICONERROR);
        _Module.Term();
        CoUninitialize();
        return 1;
    }
}
