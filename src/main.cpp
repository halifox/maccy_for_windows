#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>
#include <imm.h>

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
constexpr UINT_PTR kPasteTimerId = 3;
constexpr UINT kSearchDebounceMilliseconds = 180;

constexpr int kSearchControlId = IDC_HISTORY_SEARCH;
constexpr int kHistoryListControlId = IDC_HISTORY_LIST;

// Maccy's default history panel is about 450 px wide. Keep the Windows
// history panel compact as well so the list remains easy to scan beside the
// independent preview window. The persisted values can be changed from the
// Appearance page or by resizing the borderless popup directly.
constexpr int kMinimumPopupWidth = 320;
constexpr int kMaximumPopupWidth = 1600;
constexpr int kMinimumPopupHeight = 260;
constexpr int kMaximumPopupHeight = 1200;
constexpr int kHistoryWindowMargin = 6;
constexpr int kHistorySearchGap = 6;
constexpr int kHistorySearchHeight = 23;
constexpr int kHistorySearchIconWidth = 24;
constexpr int kHistorySearchClearWidth = 20;
constexpr int kHistoryPreviewWidth = 23;
constexpr int kHistoryHeaderGap = 6;
constexpr int kHistoryItemHeight = 22;
constexpr int kHistoryItemInset = 2;
constexpr int kHistoryItemRadius = 7;
constexpr int kHistoryItemLeftPadding = 10;
constexpr int kHistoryItemRightPadding = 10;
constexpr int kHistoryItemSlot = 16;
constexpr int kHistoryItemSlotGap = 6;
constexpr int kHistoryShortcutWidth = 74;
constexpr int kHistoryFooterHeight = 24;
constexpr int kHistoryFooterGap = 6;
constexpr int kHistorySectionGap = 6;
constexpr int kHistoryFooterCount = 4;
constexpr int kResizeBorder = 8;
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
    struct HistoryItemLayout {
        RECT background{};
        RECT icon{};
        RECT attachment{};
        RECT content{};
        RECT shortcut{};
    };

    HistoryItemLayout LayoutHistoryItem(const RECT &row, const ClipboardItem &item) const {
        HistoryItemLayout layout{};
        layout.background = row;
        layout.background.left += kHistoryItemInset;
        layout.background.right -= kHistoryItemInset;
        layout.shortcut = row;
        layout.shortcut.left = std::max(row.left, row.right - kHistoryItemRightPadding - kHistoryShortcutWidth);
        layout.shortcut.right = row.right - kHistoryItemRightPadding;
        int x = row.left + kHistoryItemLeftPadding;
        const int y = row.top + std::max<LONG>(0, ((row.bottom - row.top) - kHistoryItemSlot) / 2);
        if (m_settings.show_application_icons && !item.application.empty()) {
            layout.icon = {x, y, x + kHistoryItemSlot, y + kHistoryItemSlot};
            x += kHistoryItemSlot + kHistoryItemSlotGap;
        }
        if (item.has_image || item.has_files) {
            layout.attachment = {x, y, x + kHistoryItemSlot, y + kHistoryItemSlot};
            x += kHistoryItemSlot + kHistoryItemSlotGap;
        }
        layout.content = row;
        layout.content.left = x;
        layout.content.right = std::max<LONG>(x, layout.shortcut.left - kHistoryItemSlotGap);
        return layout;
    }

    friend struct MainWindowTests;
public:
    enum { IDD = IDD_HISTORY };

    enum class PreviewSource {
        None,
        Mouse,
        Keyboard,
    };

    explicit MainWindow(Database &database, bool isolated = false)
        : m_database(database), m_settings(AppSettings::Load(database)), m_isolated(isolated) {}

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnEditColor)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnEditColor)
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
        MESSAGE_HANDLER(WM_SYSKEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_SYSKEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnImeStart)
        MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnImeEnd)
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
        m_keyboardNavigating = false;
        CaptureTargetWindow();
        m_activeItemId = 0;
        m_previewSuppressed = false;
        m_activeFooter = -1;
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
    bool IsComposing(HWND window) const {
        HIMC context = ImmGetContext(window);
        const bool composing = context && ImmGetCompositionStringW(context, GCS_COMPSTR, nullptr, 0) > 0;
        if (context) ImmReleaseContext(window, context);
        return m_imeComposing || composing;
    }

    static LRESULT CALLBACK SearchWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto *owner = reinterpret_cast<MainWindow *>(GetPropW(window, kControlOwnerProperty));
        if (!owner) return ::DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_IME_STARTCOMPOSITION) owner->m_imeComposing = true;
        if (message == WM_IME_ENDCOMPOSITION) owner->m_imeComposing = false;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && !owner->IsComposing(window) &&
            owner->HandlePopupKey(wParam)) return 0;
        if (message == WM_KEYUP || message == WM_SYSKEYUP) owner->UpdateFooterControls();
        return CallWindowProcW(owner->m_originalSearchProc, window, message, wParam, lParam);
    }

    static LRESULT CALLBACK HistoryListWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
        auto *owner = reinterpret_cast<MainWindow *>(GetPropW(window, kControlOwnerProperty));
        if (!owner) return ::DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_MOUSEMOVE)
            owner->OnHistoryMouseMove(window, POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
        if (message == WM_MOUSELEAVE && owner->m_hoverList == window) owner->OnHistoryMouseLeave();
        if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
            if (owner->HandlePopupKey(wParam)) return 0;
        }
        if (message == WM_KEYUP || message == WM_SYSKEYUP) owner->UpdateFooterControls();
        if (message == WM_CHAR && wParam >= 0x20 && wParam != 0x7f) {
            owner->TypeToSearch(wParam); return 0;
        }
        if (message == WM_LBUTTONUP) {
            const int index = owner->HistoryItemAtPoint(window, {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)});
            if (index >= 0) owner->PasteItem(index);
            return 0;
        }
        const LRESULT result = CallWindowProcW(owner->m_originalHistoryListProc, window, message, wParam, lParam);
        if (message == WM_MOUSEWHEEL || message == WM_VSCROLL) owner->UpdateHistoryHoverFromCursor();
        return result;
    }

    static LRESULT CALLBACK MenuControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                            UINT_PTR, DWORD_PTR data) {
        auto *owner = reinterpret_cast<MainWindow *>(data);
        if (message == WM_MOUSEMOVE) {
            if (!owner->MouseCanSelect()) return DefSubclassProc(window, message, wParam, lParam);
            const auto buttons = owner->FooterButtons();
            const auto it = std::find(buttons.begin(), buttons.end(), window);
            if (it != buttons.end()) owner->SelectFooter(static_cast<int>(it - buttons.begin()));
        }
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && wParam == VK_RETURN &&
            (::GetDlgCtrlID(window) == IDC_HISTORY_PREVIEW || ::GetDlgCtrlID(window) == IDC_HISTORY_SEARCH_CLEAR)) {
            SendMessageW(owner->m_hWnd, WM_COMMAND, MAKEWPARAM(::GetDlgCtrlID(window), BN_CLICKED), reinterpret_cast<LPARAM>(window));
            return 0;
        }
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) && owner->HandlePopupKey(wParam)) return 0;
        if (message == WM_KEYUP || message == WM_SYSKEYUP) owner->UpdateFooterControls();
        if (message == WM_CHAR && wParam >= 0x20 && wParam != 0x7f) {
            owner->TypeToSearch(wParam); return 0;
        }
        return DefSubclassProc(window, message, wParam, lParam);
    }

    std::array<HWND, 4> FooterButtons() const {
        return {m_footerClear, m_footerSettings, m_footerAbout, m_footerExit};
    }

    void FocusSearchOrPopup() {
        ::SetFocus(::IsWindowVisible(m_search) ? m_search : m_hWnd);
    }

    bool MouseCanSelect() {
        POINT position{}; GetCursorPos(&position);
        if (m_keyboardNavigating && position.x == m_keyboardPointer.x && position.y == m_keyboardPointer.y) return false;
        m_keyboardNavigating = false;
        return true;
    }

    void TypeToSearch(WPARAM character) {
        if (!m_settings.show_search) return;
        SetHistorySearchVisible(true);
        ::SetFocus(m_search);
        const wchar_t text[] = {static_cast<wchar_t>(character), 0};
        SendMessageW(m_search, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text));
    }

    void SelectFooter(int index) {
        if (m_activeFooter == index) return;
        const int old = m_activeItemIndex;
        m_activeFooter = index;
        InvalidateHistoryItem(old);
        for (HWND button : FooterButtons()) ::InvalidateRect(button, nullptr, FALSE);
    }

    void TogglePreview() {
        if (m_previewWindow.IsVisible()) {
            m_previewSuppressed = true;
            HidePreview();
        } else {
            m_previewSuppressed = false;
            ShowPreviewForSelection();
        }
    }

    bool HandlePopupKey(WPARAM key) {
        UpdateFooterControls();
        if (m_search && (key == VK_RETURN || key == VK_UP || key == VK_DOWN || key == VK_PRIOR || key == VK_NEXT ||
            (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000))) {
            const auto query = ReadWindowText(m_search);
            if (query != m_searchQuery) { KillTimer(kSearchTimerId); RefreshHistory(query); }
        }
        if (HandlePopupShortcut(key)) return true;
        const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (key == VK_ESCAPE) { HideMainWindow(); return true; }
        if (key == VK_RETURN) {
            if (m_activeFooter >= 0) {
                const auto buttons = FooterButtons();
                SendMessageW(m_hWnd, WM_COMMAND, MAKEWPARAM(::GetDlgCtrlID(buttons[m_activeFooter]), BN_CLICKED),
                             reinterpret_cast<LPARAM>(buttons[m_activeFooter]));
            } else PasteSelectedItem();
            return true;
        }
        if (key == VK_UP || key == VK_DOWN || key == VK_TAB) {
            NavigateHistoryFromSearch(key == VK_DOWN || (key == VK_TAB && !(GetKeyState(VK_SHIFT) & 0x8000)));
            return true;
        }
        if ((ctrl && (key == VK_HOME || key == VK_END)) || key == VK_PRIOR || key == VK_NEXT) {
            m_keyboardNavigating = true; GetCursorPos(&m_keyboardPointer);
            if (!m_items.empty()) SetActiveHistoryItem(key == VK_HOME || key == VK_PRIOR ? 0 : static_cast<int>(m_items.size()) - 1);
            FocusSearchOrPopup();
            return true;
        }
        if (ctrl && key == 'F' && m_settings.show_search) {
            SetHistorySearchVisible(true); ::SetFocus(m_search); return true;
        }
        if (ctrl && key == 'U') {
            ::SetWindowTextW(m_search, L""); RefreshHistory(L""); return true;
        }
        return false;
    }

    bool BindControls() {
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
        SendMessageW(::GetDlgItem(m_hWnd, IDC_HISTORY_TITLE), WM_SETFONT, reinterpret_cast<WPARAM>(m_smallFont), TRUE);
        for (HWND control : {m_search, m_historyList, m_footerClear, m_footerSettings, m_footerAbout, m_footerExit}) {
            SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
        SetPropW(m_pinsList, kControlOwnerProperty, reinterpret_cast<HANDLE>(this));
        m_originalPinsProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(m_pinsList, GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&MainWindow::HistoryListWindowProc)));
        SendMessageW(m_pinsList, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        for (HWND button : FooterButtons()) {
            SetWindowSubclass(button, MenuControlProc, 1, reinterpret_cast<DWORD_PTR>(this));
        }
        for (HWND button : {m_searchClear, m_previewToggle}) {
            SendMessageW(button, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
            SetWindowSubclass(button, MenuControlProc, 1, reinterpret_cast<DWORD_PTR>(this));
        }
        // EDITTEXT/LISTBOX resource macros add a border implicitly.
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
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, m_hWnd, nullptr, _Module.GetModuleInstance(), nullptr);
        for (HWND control : {m_searchClear, m_previewToggle}) {
            TOOLINFOW info{sizeof(info)};
            info.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
            info.hwnd = m_hWnd; info.uId = reinterpret_cast<UINT_PTR>(control);
            info.lpszText = const_cast<wchar_t *>(control == m_searchClear ? L"清除搜索（Ctrl+U）" : m_previewTip.c_str());
            SendMessageW(m_tooltips, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&info));
        }
        LONG_PTR search_style = ::GetWindowLongPtrW(m_search, GWL_STYLE);
        search_style &= ~static_cast<LONG_PTR>(ES_MULTILINE);
        search_style |= ES_AUTOHSCROLL;
        ::SetWindowLongPtrW(m_search, GWL_STYLE, search_style);
        SendMessageW(m_search, EM_SETCUEBANNER, TRUE, reinterpret_cast<LPARAM>(L"搜索剪贴板内容…"));
        RECT search_rect{};
        if (::GetWindowRect(m_search, &search_rect)) {
            m_searchHeight = std::max(1L, search_rect.bottom - search_rect.top);
        }
        return true;
    }

    void LayoutHistoryControls() {
        if (!m_search || !m_historyList || !::IsWindow(m_hWnd)) return;
        RECT client{}; ::GetClientRect(m_hWnd, &client);
        const int margin = kHistoryWindowMargin;
        const int width = std::max(1L, client.right - 2 * margin);
        const bool header = ::IsWindowVisible(m_search) != FALSE;
        const int headerHeight = header ? kHistorySearchHeight : 0;
        int titleWidth = 0;
        if (header && m_settings.show_title) {
            HDC dc = ::GetDC(m_hWnd);
            if (dc != nullptr) {
                const HFONT font = m_smallFont != nullptr ? m_smallFont : m_normalFont;
                const HGDIOBJ oldFont = font != nullptr ? ::SelectObject(dc, font) : nullptr;
                SIZE textSize{};
                if (::GetTextExtentPoint32W(dc, L"Clipboard", 9, &textSize)) {
                    titleWidth = textSize.cx + 8;
                }
                if (oldFont != nullptr) ::SelectObject(dc, oldFont);
                ::ReleaseDC(m_hWnd, dc);
            }
        }
        m_titleRect = {margin, margin, margin + titleWidth, margin + headerHeight};
        const HWND title = ::GetDlgItem(m_hWnd, IDC_HISTORY_TITLE);
        ::SetWindowPos(title, nullptr, margin, margin, titleWidth, headerHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        ::ShowWindow(title, titleWidth ? SW_SHOW : SW_HIDE);
        const int previewLeft = margin + width - kHistoryPreviewWidth;
        const int searchLeft = margin + titleWidth + (titleWidth ? kHistoryHeaderGap : 0);
        const int searchRight = previewLeft - kHistoryHeaderGap;
        m_searchRect = {searchLeft, margin, searchRight, margin + headerHeight};
        if (header) {
            ::SetWindowPos(m_search, nullptr, searchLeft + kHistorySearchIconWidth, margin,
                std::max<LONG>(1L, searchRight - searchLeft - kHistorySearchIconWidth - kHistorySearchClearWidth),
                kHistorySearchHeight,
                SWP_NOZORDER | SWP_NOACTIVATE);
            ::SetWindowPos(m_searchClear, nullptr, searchRight - kHistorySearchClearWidth, margin,
                kHistorySearchClearWidth,
                kHistorySearchHeight, SWP_NOZORDER | SWP_NOACTIVATE);
            ::SetWindowPos(m_previewToggle, nullptr, previewLeft, margin, kHistoryPreviewWidth,
                kHistorySearchHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        } else {
            for (HWND control : {m_search, m_searchClear, m_previewToggle}) {
                ::SetWindowPos(control, nullptr, 0, 0, 0, 0,
                    SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
            }
        }
        ::ShowWindow(m_searchClear, header && !ReadWindowText(m_search).empty() ? SW_SHOW : SW_HIDE);
        ::ShowWindow(m_previewToggle, header ? SW_SHOW : SW_HIDE);
        int top = margin + (header ? headerHeight + kHistorySearchGap : 0);
        const int footerHeight = m_settings.show_footer
            ? kHistoryFooterGap + kHistoryFooterHeight * kHistoryFooterCount
            : 0;
        const int bottom = std::max(top + 1, static_cast<int>(client.bottom) - margin - footerHeight);
        const int available = std::max(1, bottom - top);
        const int pinCount = static_cast<int>(SendMessageW(m_pinsList, LB_GETCOUNT, 0, 0));
        const bool havePins = pinCount > 0;
        const bool haveHistory = SendMessageW(m_historyList, LB_GETCOUNT, 0, 0) > 0;
        const int gap = havePins && haveHistory ? kHistorySectionGap : 0;
        const int requestedPinsHeight = pinCount * kHistoryItemHeight;
        const int pinsHeight = havePins
            ? std::min(requestedPinsHeight, haveHistory
                ? std::max(1, available - gap - kHistoryItemHeight)
                : available)
            : 0;
        const int historyHeight = haveHistory
            ? std::max(1, available - pinsHeight - gap)
            : 1;
        const bool bottomPins = m_settings.pin_to == PinPosition::Bottom;
        const int pinTop = bottomPins && haveHistory ? top + historyHeight + gap : top;
        const int historyTop = !bottomPins && havePins ? top + pinsHeight + gap : top;
        ::SetWindowPos(m_pinsList, nullptr, margin, pinTop, width, pinsHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        ::ShowWindow(m_pinsList, havePins ? SW_SHOW : SW_HIDE);
        ::SetWindowPos(m_historyList, nullptr, margin, historyTop, width, historyHeight, SWP_NOZORDER | SWP_NOACTIVATE);
        ::ShowWindow(m_historyList, haveHistory || !havePins ? SW_SHOW : SW_HIDE);
        m_pinSeparatorY = gap ? (bottomPins ? pinTop - gap / 2 : historyTop - gap / 2) : -1;
        m_footerSeparatorY = m_settings.show_footer ? bottom + 5 : -1;
        const auto buttons = FooterButtons();
        for (int i = 0; i < 4; ++i) {
            ::SetWindowPos(buttons[i], nullptr, margin,
                           bottom + kHistoryFooterGap + i * kHistoryFooterHeight,
                           width, kHistoryFooterHeight, SWP_NOZORDER | SWP_NOACTIVATE);
            ::ShowWindow(buttons[i], m_settings.show_footer ? SW_SHOW : SW_HIDE);
        }
        UpdateFooterControls();
        ::InvalidateRect(m_hWnd, nullptr, TRUE);
    }

    void ApplyHistoryVisibility() {
        if (m_search == nullptr) {
            return;
        }
        const bool show_search = m_settings.show_search &&
            (m_settings.search_visibility == SearchVisibility::Always || !m_searchQuery.empty());
        SetHistorySearchVisible(show_search);
        LayoutHistoryControls();
    }

    void SetHistorySearchVisible(bool visible) {
        if (m_search == nullptr || !::IsWindow(m_search)) {
            return;
        }
        ::ShowWindow(m_search, visible ? SW_SHOW : SW_HIDE);
        LayoutHistoryControls();
        if (m_popupVisible && !m_inSizeMove) {
            RECT rect{};
            if (::GetWindowRect(m_hWnd, &rect)) {
                ::SetWindowPos(m_hWnd, nullptr, rect.left, rect.top,
                    rect.right - rect.left, PopupHeight(),
                    SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }

    void UpdateFooterControls() {
        if (!m_footerClear) return;
        const bool all = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const std::wstring caption = all ? L"清空全部历史" : L"清空历史";
        if (ReadWindowText(m_footerClear) != caption) ::SetWindowTextW(m_footerClear, caption.c_str());
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
        RestoreControlSubclass(m_pinsList, m_originalPinsProc);
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

    int PopupWidth() const {
        return std::clamp(m_settings.window_width, kMinimumPopupWidth, kMaximumPopupWidth);
    }

    int PopupHeight() const {
        const int maximum = std::clamp(m_settings.window_height, kMinimumPopupHeight, kMaximumPopupHeight);
        const bool header = m_settings.show_search &&
            (m_settings.search_visibility == SearchVisibility::Always || !m_searchQuery.empty());
        int pinRows = 0;
        for (const ClipboardItem &item : m_items) if (item.pinned) ++pinRows;
        const int historyRows = static_cast<int>(m_items.size()) - pinRows;
        const int sectionGap = pinRows > 0 && historyRows > 0 ? kHistorySectionGap : 0;
        const int listRows = std::max(3, historyRows) + pinRows;
        const int footerHeight = m_settings.show_footer
            ? kHistoryFooterGap + kHistoryFooterHeight * kHistoryFooterCount
            : 0;
        const int content = 2 * kHistoryWindowMargin +
            (header ? kHistorySearchHeight + kHistorySearchGap : 0) +
            sectionGap + listRows * kHistoryItemHeight + footerHeight;
        const int previewMinimum = m_previewWindow.IsVisible() ? m_previewWindow.MinimumHeight() : 0;
        return std::clamp(std::max(content, previewMinimum), kMinimumPopupHeight, maximum);
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
            x = work_area.left + ((work_area.right - work_area.left) - PopupWidth()) / 2;
            y = work_area.top + ((work_area.bottom - work_area.top) - PopupHeight()) / 2;
        }
        const LONG max_x = std::max<LONG>(work_area.left, work_area.right - PopupWidth());
        const LONG max_y = std::max<LONG>(work_area.top, work_area.bottom - PopupHeight());
        x = std::clamp(x, static_cast<int>(work_area.left), static_cast<int>(max_x));
        y = std::clamp(y, static_cast<int>(work_area.top), static_cast<int>(max_y));
        ::SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, PopupWidth(), PopupHeight(), SWP_NOACTIVATE);
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
        RECT target{};
        const bool has_target = m_targetWindow != nullptr
            && ::IsWindow(m_targetWindow)
            && ::GetWindowRect(m_targetWindow, &target) == TRUE;
        const HMONITOR monitor = m_settings.popup_position == PopupPosition::WindowTopLeft && has_target
            ? ::MonitorFromWindow(m_targetWindow, MONITOR_DEFAULTTONEAREST)
            : SelectedMonitor();
        MONITORINFO monitor_info{sizeof(monitor_info)};
        if (monitor == nullptr || !GetMonitorInfoW(monitor, &monitor_info)) {
            return;
        }
        const RECT &work_area = monitor_info.rcWork;
        int x = cursor.x;
        int y = cursor.y - PopupHeight();

        switch (m_settings.popup_position) {
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
        case PopupPosition::WindowTopLeft:
            if (has_target) {
                x = target.left;
                y = target.top;
            }
            break;
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
            break;
        }

        const LONG max_x = std::max<LONG>(work_area.left, work_area.right - PopupWidth());
        const LONG max_y = std::max<LONG>(work_area.top, work_area.bottom - PopupHeight());
        x = std::clamp(x, static_cast<int>(work_area.left), static_cast<int>(max_x));
        y = std::clamp(y, static_cast<int>(work_area.top), static_cast<int>(max_y));
        ::SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, PopupWidth(), PopupHeight(), SWP_NOACTIVATE);
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
        if (PasteModifiersDown()) {
            m_pendingPasteTarget = target;
            m_pendingPasteFocus = target_focus;
            m_pendingPasteDeadline = GetTickCount64() + 3000;
            ::SetTimer(m_hWnd, kPasteTimerId, 15, nullptr);
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

    static bool PasteModifiersDown() {
        return (GetAsyncKeyState(VK_CONTROL) & 0x8000) ||
               (GetAsyncKeyState(VK_MENU) & 0x8000) || (GetAsyncKeyState(VK_SHIFT) & 0x8000);
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

    int ItemIndex(HWND list, int row) const {
        if (row < 0) return -1;
        const LRESULT value = SendMessageW(list, LB_GETITEMDATA, row, 0);
        return value == LB_ERR ? -1 : static_cast<int>(value);
    }

    HWND ListForItem(int index) const { return m_items[index].pinned ? m_pinsList : m_historyList; }

    int RowForItem(int index) const {
        if (index < 0 || static_cast<size_t>(index) >= m_items.size()) return -1;
        HWND list = ListForItem(index);
        const int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
        for (int row = 0; row < count; ++row) if (ItemIndex(list, row) == index) return row;
        return -1;
    }

    int HistoryItemAtPoint(HWND window, POINT point) const {
        if (window != m_historyList && window != m_pinsList) return -1;
        const LRESULT hit = SendMessageW(window, LB_ITEMFROMPOINT, 0, MAKELPARAM(point.x, point.y));
        return HIWORD(hit) ? -1 : ItemIndex(window, LOWORD(hit));
    }

    void InvalidateHistoryItem(int index) {
        const int row = RowForItem(index);
        if (row < 0) return;
        RECT rect{};
        HWND list = ListForItem(index);
        if (SendMessageW(list, LB_GETITEMRECT, row, reinterpret_cast<LPARAM>(&rect)) != LB_ERR)
            ::InvalidateRect(list, &rect, FALSE);
    }

    void BeginHistoryMouseTracking() {
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, m_hoverList, 0};
        ::TrackMouseEvent(&tracking);
    }

    void SetActiveHistoryItem(int index) {
        if (index < 0 || static_cast<size_t>(index) >= m_items.size()) return;
        const int previous = m_activeItemIndex;
        m_activeItemIndex = index;
        m_activeItemId = m_items[index].id;
        m_activeFooter = -1;
        m_mouseSelectionUpdate = true;
        SendMessageW(m_historyList, LB_SETCURSEL, -1, 0);
        SendMessageW(m_pinsList, LB_SETCURSEL, -1, 0);
        SendMessageW(ListForItem(index), LB_SETCURSEL, RowForItem(index), 0);
        m_mouseSelectionUpdate = false;
        InvalidateHistoryItem(previous);
        InvalidateHistoryItem(index);
        for (HWND button : FooterButtons()) ::InvalidateRect(button, nullptr, FALSE);
    }

    void OnHistoryMouseMove(HWND window, POINT point) {
        if (!MouseCanSelect()) return;
        if (!m_popupVisible || (window != m_historyList && window != m_pinsList)) {
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
        m_hoverList = window;
        BeginHistoryMouseTracking();

        const int index = HistoryItemAtPoint(window, point);
        if (index < 0) {
            ClearHistoryHover(true);
            return;
        }

        const sqlite3_int64 item_id = m_items[static_cast<size_t>(index)].id;
        if (index == m_hoveredItemIndex && item_id == m_hoveredItemId) {
            SetActiveHistoryItem(index);
            if (!m_previewWindow.IsVisible() && m_previewCandidateId == 0) {
                SchedulePreviewForItem(item_id, PreviewSource::Mouse);
            }
            return;
        }

        const int previous_index = m_hoveredItemIndex;
        m_hoveredItemIndex = index;
        m_hoveredItemId = item_id;
        SetActiveHistoryItem(index);

        InvalidateHistoryItem(previous_index);
        InvalidateHistoryItem(index);

        m_previewSource = PreviewSource::Mouse;
        if (m_previewWindow.IsVisible()) {
            ShowPreviewForItem(item_id);
        } else {
            SchedulePreviewForItem(item_id, PreviewSource::Mouse);
        }
    }

    void OnHistoryMouseLeave() {
        m_historyMouseTracking = false;
        ClearHistoryHover(true);
    }

    void UpdateHistoryHoverFromCursor() {
        if (!m_popupVisible) return;
        POINT cursor{}; if (!GetCursorPos(&cursor)) return;
        for (HWND list : {m_historyList, m_pinsList}) {
            if (!::IsWindowVisible(list)) continue;
            POINT point = cursor; ::ScreenToClient(list, &point);
            RECT rect{}; ::GetClientRect(list, &rect);
            if (::PtInRect(&rect, point)) { OnHistoryMouseMove(list, point); return; }
        }
        ClearHistoryHover();
    }

    void ClearHistoryHover(bool = false) {
        m_hoveredItemIndex = -1;
        m_hoveredItemId = 0;
        // The preview is a reading pane, not a tooltip. It stays open when
        // the pointer crosses from the history list into its content.
        KillTimer(kPreviewTimerId);
        m_previewCandidateId = 0;
    }

    void RefreshHistory(std::wstring_view query) {
        try {
            const bool sameQuery = query == m_searchQuery;
            const sqlite3_int64 previous = sameQuery ? m_activeItemId : 0;
            const int previousIndex = m_activeItemIndex;
            const bool previewOpen = m_previewWindow.IsVisible();
            const std::wstring ownedQuery(query);
            KillTimer(kPreviewTimerId);
            m_previewCandidateId = 0;
            m_hoveredItemIndex = -1; m_hoveredItemId = 0;
            m_activeItemIndex = -1; m_activeItemId = 0; m_activeFooter = -1;
            m_searchQuery = ownedQuery;
            m_items = m_database.SearchHistory(ownedQuery, static_cast<int>(m_settings.search_mode),
                m_settings.sort_by, m_settings.pin_to == PinPosition::Bottom);
            m_loadingList = true;
            for (HWND list : {m_historyList, m_pinsList}) {
                SendMessageW(list, WM_SETREDRAW, FALSE, 0);
                SendMessageW(list, LB_RESETCONTENT, 0, 0);
            }
            int selected = -1;
            for (size_t index = 0; index < m_items.size(); ++index) {
                const auto &item = m_items[index];
                HWND list = ListForItem(static_cast<int>(index));
                const std::wstring display = DisplayText(item);
                const LRESULT row = SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(display.c_str()));
                SendMessageW(list, LB_SETITEMDATA, row, index);
                if (item.id == previous) selected = static_cast<int>(index);
            }
            if (selected < 0 && !m_items.empty()) {
                if (previous && sameQuery) selected = std::clamp(previousIndex, 0, static_cast<int>(m_items.size()) - 1);
                else {
                    selected = 0;
                    if (ownedQuery.empty()) {
                        auto it = std::find_if(m_items.begin(), m_items.end(), [](const auto &i) { return !i.pinned; });
                        if (it != m_items.end()) selected = static_cast<int>(it - m_items.begin());
                    }
                }
            }
            if (selected >= 0) SetActiveHistoryItem(selected);
            m_loadingList = false;
            for (HWND list : {m_historyList, m_pinsList}) {
                SendMessageW(list, WM_SETREDRAW, TRUE, 0);
                ::InvalidateRect(list, nullptr, TRUE);
            }
            ApplyHistoryVisibility();
            if (m_popupVisible && !m_inSizeMove) {
                RECT rect{}; ::GetWindowRect(m_hWnd, &rect);
                ::SetWindowPos(m_hWnd, nullptr, 0, 0, rect.right - rect.left, PopupHeight(),
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            if (previewOpen && m_activeItemId) ShowPreviewForItem(m_activeItemId);
            else if (!m_activeItemId) HidePreview();
        } catch (const std::exception &error) {
            m_loadingList = false;
            for (HWND list : {m_historyList, m_pinsList}) SendMessageW(list, WM_SETREDRAW, TRUE, 0);
            OutputDebugStringA(error.what());
        }
    }

    void ScheduleSearch() {
        KillTimer(kSearchTimerId);
        ::SetTimer(m_hWnd, kSearchTimerId, kSearchDebounceMilliseconds, nullptr);
    }

    void SchedulePreviewForItem(sqlite3_int64 item_id, PreviewSource source) {
        KillTimer(kPreviewTimerId);
        m_previewCandidateId = 0;
        if (!m_popupVisible || m_previewSuppressed || !m_settings.open_preview_automatically ||
            item_id == 0 || m_previewWindow.IsVisible()) {
            return;
        }
        m_previewSource = source;
        m_previewCandidateId = item_id;
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
            if (m_popupVisible && !m_inSizeMove) {
                RECT rect{};
                if (::GetWindowRect(m_hWnd, &rect)) {
                    ::SetWindowPos(m_hWnd, nullptr, rect.left, rect.top,
                        rect.right - rect.left, PopupHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
                }
            }
        } catch (...) {
            HidePreview();
        }
    }

    void ShowPreviewForCandidate() {
        if (m_popupVisible && m_previewCandidateId && m_previewCandidateId == m_activeItemId && !m_previewSuppressed)
            ShowPreviewForItem(m_previewCandidateId);
    }

    void ShowPreviewForSelection() {
        const int selected = SelectedHistoryIndex();
        if (selected < 0) {
            HidePreview();
            return;
        }
        SetActiveHistoryItem(selected);
        m_previewSource = PreviewSource::Keyboard;
        ShowPreviewForItem(m_activeItemId);
    }

    void HidePreview() {
        KillTimer(kPreviewTimerId);
        m_previewCandidateId = 0;
        m_previewItemId = 0;
        m_previewSource = PreviewSource::None;
        m_previewWindow.Hide();
        if (m_popupVisible && !m_inSizeMove) {
            RECT rect{};
            if (::GetWindowRect(m_hWnd, &rect)) {
                ::SetWindowPos(m_hWnd, nullptr, rect.left, rect.top,
                    rect.right - rect.left, PopupHeight(), SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
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
                ranges.emplace_back(found, found + 1);
                text_position = found + 1;
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
        const int saved = SaveDC(dc);
        IntersectClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
        const auto originalRanges = HighlightRanges(text);
        const auto measure = [&](std::wstring_view value) {
            SIZE size{};
            // Use the bold font for fitting so highlighted text also fits.
            SelectObject(dc, m_boldFont);
            GetTextExtentPoint32W(dc, value.data(), static_cast<int>(value.size()), &size);
            return size.cx;
        };
        std::wstring display(text);
        size_t prefix = text.size(), suffix = 0;
        if (measure(text) > rect.right - rect.left) {
            size_t low = 0, high = text.size();
            while (low < high) {
                const size_t length = (low + high + 1) / 2;
                const auto candidate = std::wstring(text.substr(0, (length + 1) / 2)) + L"…" + std::wstring(text.substr(text.size() - length / 2));
                if (measure(candidate) <= rect.right - rect.left) low = length; else high = length - 1;
            }
            prefix = (low + 1) / 2; suffix = low / 2;
            if (prefix && text[prefix - 1] >= 0xd800 && text[prefix - 1] <= 0xdbff) --prefix;
            if (suffix && text[text.size() - suffix] >= 0xdc00 && text[text.size() - suffix] <= 0xdfff) --suffix;
            display = std::wstring(text.substr(0, prefix)) + L"…" + std::wstring(text.substr(text.size() - suffix));
        }
        std::vector<std::pair<size_t, size_t>> ranges;
        for (const auto &[start, end] : originalRanges) {
            if (start < prefix) ranges.emplace_back(start, std::min(end, prefix));
            if (suffix && end > text.size() - suffix) {
                const size_t from = std::max(start, text.size() - suffix);
                ranges.emplace_back(prefix + 1 + from - (text.size() - suffix), prefix + 1 + end - (text.size() - suffix));
            }
        }
        text = display;
        SelectObject(dc, m_normalFont);
        SetTextColor(dc, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
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
        RestoreDC(dc, saved);
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

    void DrawMenuButton(DRAWITEMSTRUCT *draw) {
        const auto buttons = FooterButtons();
        auto it = std::find(buttons.begin(), buttons.end(), draw->hwndItem);
        const int index = it == buttons.end() ? -1 : static_cast<int>(it - buttons.begin());
        const bool selected = index >= 0 && index == m_activeFooter;
        FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_WINDOW));
        if (selected || (draw->itemState & ODS_SELECTED)) {
            auto pen = SelectObject(draw->hDC, GetStockObject(NULL_PEN));
            auto brush = SelectObject(draw->hDC, GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
            RoundRect(draw->hDC, 0, 0, draw->rcItem.right, draw->rcItem.bottom, 8, 8);
            SelectObject(draw->hDC, brush); SelectObject(draw->hDC, pen);
        }
        SelectObject(draw->hDC, m_normalFont);
        SetBkMode(draw->hDC, TRANSPARENT);
        SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
        RECT rect = draw->rcItem; rect.left += index < 0 ? 2 : 10; rect.right -= index < 0 ? 2 : 10;
        if (draw->CtlID == IDC_HISTORY_PREVIEW) {
            HPEN pen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
            auto oldPen = SelectObject(draw->hDC, pen);
            auto oldBrush = SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(draw->hDC, 5, 5, 22, 19);
            MoveToEx(draw->hDC, 15, 5, nullptr); LineTo(draw->hDC, 15, 19);
            SelectObject(draw->hDC, oldBrush); SelectObject(draw->hDC, oldPen); DeleteObject(pen);
            return;
        }
        const auto title = draw->CtlID == IDC_HISTORY_SEARCH_CLEAR ? std::wstring(L"×") : ReadWindowText(draw->hwndItem);
        DrawTextW(draw->hDC, title.c_str(), -1, &rect, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (index < 0 ? DT_CENTER : DT_LEFT));
        if (index >= 0) {
            const wchar_t *keys[] = {(GetKeyState(VK_SHIFT) & 0x8000) ? L"Ctrl+Alt+Shift+Backspace" : L"Ctrl+Alt+Backspace", L"Ctrl+,", L"", L"Ctrl+Q"};
            SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT));
            DrawTextW(draw->hDC, keys[index], -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
    }

    void DrawHistoryItem(DRAWITEMSTRUCT *draw) {
        if (draw == nullptr || draw->itemID == static_cast<UINT>(-1) ||
            static_cast<size_t>(draw->itemData) >= m_items.size()) {
            return;
        }
        const int index = static_cast<int>(draw->itemData);
        const ClipboardItem &item = m_items[index];
        const bool selected = m_activeFooter < 0 && index == m_activeItemIndex;
        const HistoryItemLayout layout = LayoutHistoryItem(draw->rcItem, item);
        FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_WINDOW));
        if (selected) {
            HGDIOBJ pen = SelectObject(draw->hDC, GetStockObject(NULL_PEN));
            HGDIOBJ brush = SelectObject(draw->hDC, GetSysColorBrush(COLOR_HIGHLIGHT));
            RECT selected_rect = layout.background;
            const bool previous_selected = index > 0 && index - 1 == m_activeItemIndex;
            const bool next_selected = index + 1 < static_cast<int>(m_items.size()) && index + 1 == m_activeItemIndex;
            if (previous_selected) selected_rect.top = draw->rcItem.top;
            if (next_selected) selected_rect.bottom = draw->rcItem.bottom;
            RoundRect(draw->hDC, selected_rect.left, selected_rect.top, selected_rect.right, selected_rect.bottom,
                kHistoryItemRadius, kHistoryItemRadius);
            if (previous_selected || next_selected) {
                RECT join = selected_rect;
                join.left += kHistoryItemRadius / 2;
                join.right -= kHistoryItemRadius / 2;
                FillRect(draw->hDC, &join, GetSysColorBrush(COLOR_HIGHLIGHT));
            }
            SelectObject(draw->hDC, brush); SelectObject(draw->hDC, pen);
        }
        SelectObject(draw->hDC, m_normalFont);

        TEXTMETRICW metrics{};
        const int text_height = GetTextMetricsW(draw->hDC, &metrics) != FALSE && metrics.tmHeight > 0
            ? metrics.tmHeight
            : 16;
        const int row_height = std::max(1L, draw->rcItem.bottom - draw->rcItem.top);
        const int text_top = draw->rcItem.top + std::max(0, (row_height - text_height) / 2);
        const int icon_top = draw->rcItem.top + std::max(0, (row_height - 16) / 2);
        RECT text_rect = layout.content;
        text_rect.top = text_top;
        text_rect.bottom = text_top + text_height;

        if (!IsRectEmpty(&layout.icon)) {
            if (const HICON icon = IconForApplication(item.application)) {
                DrawIconEx(draw->hDC, layout.icon.left, layout.icon.top, icon, 16, 16, 0, nullptr, DI_NORMAL);
            }
        }
        if (!IsRectEmpty(&layout.attachment)) {
            HBRUSH brush = CreateSolidBrush(item.has_image ? RGB(225, 230, 235) : RGB(250, 220, 130));
            FillRect(draw->hDC, &layout.attachment, brush);
            DeleteObject(brush);
            FrameRect(draw->hDC, &layout.attachment, GetSysColorBrush(COLOR_GRAYTEXT));
        }

        std::wstring shortcut;
        if (item.pinned) shortcut = L"Ctrl+" + item.pin;
        else {
            int number = 0;
            for (int i = 0; i <= index; ++i) if (!m_items[i].pinned) ++number;
            if (number <= 9) shortcut = L"Ctrl+" + std::to_wstring(number);
        }
        RECT keyRect = layout.shortcut;
        SetBkMode(draw->hDC, TRANSPARENT);
        SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT));
        DrawTextW(draw->hDC, shortcut.c_str(), -1, &keyRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        text_rect.right = layout.content.right;
        const std::wstring text = DisplayText(item);
        DrawTextWithHighlights(draw->hDC, text_rect, text, selected);
        if (m_settings.show_hex_color_swatch) {
            const size_t hash = text.find(L'#');
            if (hash != std::wstring::npos && hash + 7 == text.size() && hash == 0) {
                const std::wstring hex = text.substr(hash + 1, 6);
                wchar_t *end = nullptr;
                const unsigned long value = wcstoul(hex.c_str(), &end, 16);
                if (end == hex.c_str() + 6) {
                    const COLORREF color = RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
                    RECT swatch{keyRect.left - 22, icon_top, keyRect.left - 6, icon_top + 16};
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
        log_font.lfHeight = -13;
        log_font.lfWeight = FW_NORMAL;
        log_font.lfCharSet = DEFAULT_CHARSET;
        log_font.lfQuality = CLEARTYPE_QUALITY;
        wcscpy_s(log_font.lfFaceName, L"Segoe UI");
        m_normalFont = CreateFontIndirectW(&log_font);

        LOGFONTW small_log_font = log_font;
        small_log_font.lfWeight = FW_NORMAL;
        if (small_log_font.lfHeight < 0) {
            small_log_font.lfHeight = std::max<LONG>(-1, (small_log_font.lfHeight * 4) / 5);
        } else {
            small_log_font.lfHeight = std::max<LONG>(1, (small_log_font.lfHeight * 4) / 5);
        }
        m_smallFont = CreateFontIndirectW(&small_log_font);

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
        for (HFONT font : {m_normalFont, m_smallFont, m_boldFont, m_italicFont, m_underlineFont}) {
            if (font != nullptr) {
                DeleteObject(font);
            }
        }
        m_smallFont = nullptr;
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

    void SaveWindowGeometry(bool resized = false) {
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
        m_settings.popup_x = rect.left;
        m_settings.popup_y = rect.top;
        m_settings.window_width = width;
        m_settings.window_height = height;
        try {
            m_database.SetSetting(L"appearance.windowX", std::to_wstring(m_settings.popup_x));
            m_database.SetSetting(L"appearance.windowY", std::to_wstring(m_settings.popup_y));
            m_database.SetSetting(L"appearance.windowWidth", std::to_wstring(width));
            m_database.SetSetting(L"appearance.windowHeight", std::to_wstring(height));
        } catch (...) {
        }
    }

    void HideMainWindow() {
        if (!m_popupVisible) {
            return;
        }
        SaveWindowGeometry();
        m_popupVisible = false;
        HidePreview();
        ShowWindow(SW_HIDE);
    }

    bool HasHistoryItems() const {
        return !m_items.empty();
    }

    void NavigateHistoryFromSearch(bool forward) {
        m_keyboardNavigating = true; GetCursorPos(&m_keyboardPointer);
        if (!m_popupVisible) return;
        const int count = static_cast<int>(m_items.size());
        const int total = count + (m_settings.show_footer ? 4 : 0);
        if (!total) return;
        int current = m_activeFooter >= 0 ? count + m_activeFooter : m_activeItemIndex;
        const int target = std::clamp(current + (forward ? 1 : -1), 0, total - 1);
        if (target >= count) SelectFooter(target - count);
        else {
            m_hoveredItemIndex = -1; m_hoveredItemId = 0;
            SetActiveHistoryItem(target);
            m_previewSource = PreviewSource::Keyboard;
            if (m_previewWindow.IsVisible()) ShowPreviewForItem(m_activeItemId);
            else SchedulePreviewForItem(m_activeItemId, PreviewSource::Keyboard);
        }
        FocusSearchOrPopup();
    }

    int SelectedHistoryIndex() const {
        return m_activeFooter < 0 ? m_activeItemIndex : -1;
    }

    std::wstring NextPinKey() const {
        constexpr wchar_t keys[] = L"bdeghijklmnorstw";
        const auto pins = m_database.SearchHistory(L"", 0, 0, false);
        for (const wchar_t key : std::wstring_view(keys)) {
            if (towupper(key) == m_settings.pin_hotkey.virtual_key ||
                towupper(key) == m_settings.delete_hotkey.virtual_key ||
                towupper(key) == m_settings.preview_hotkey.virtual_key) continue;
            bool used = false;
            for (const ClipboardItem &item : pins) {
                if (item.pinned && item.pin.size() == 1 && item.pin[0] == key) {
                    used = true;
                    break;
                }
            }
            if (!used) {
                return std::wstring(1, key);
            }
        }
        return L"";
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
                const auto key = NextPinKey();
                if (key.empty()) {
                    MessageBoxW(L"没有可用的置顶快捷键，请先取消一个置顶项目。", L"置顶", MB_OK);
                    return;
                }
                m_database.TogglePin(item.id, key, true);
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
        if (IsHotKeyPressed(m_settings.pin_hotkey, key)) { ToggleSelectedPin(); return true; }
        if (IsHotKeyPressed(m_settings.delete_hotkey, key)) { DeleteSelectedItem(); return true; }
        if (IsHotKeyPressed(m_settings.preview_hotkey, key)) { TogglePreview(); return true; }
        const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
        const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        if (ctrl && alt && key == VK_BACK) { ClearHistory(shift); return true; }
        if (ctrl && !alt && key == VK_OEM_COMMA) { OpenSettings(); return true; }
        if (ctrl && !alt && key == 'Q') { ExitApplication(); return true; }
        // Keep native Edit shortcuts and global popup navigation available.
        if (ctrl && !alt && std::wstring_view(L"ACFUVXYZ").find(static_cast<wchar_t>(key)) != std::wstring_view::npos) return false;
        if ((ctrl != alt) && !(GetKeyState(VK_LWIN) & 0x8000) && !(GetKeyState(VK_RWIN) & 0x8000)) {
            int number = 0;
            for (size_t i = 0; i < m_items.size(); ++i) {
                const auto &item = m_items[i];
                const bool matches = item.pinned
                    ? item.pin.size() == 1 && towupper(item.pin[0]) == key
                    : (++number <= 9 && key == static_cast<WPARAM>('0' + number));
                if (matches) { SetActiveHistoryItem(static_cast<int>(i)); PasteItem(static_cast<int>(i)); return true; }
            }
        }
        return false;
    }

    static std::pair<bool, bool> ResolvePasteAction(bool paste, bool plain, bool alt, bool shift) {
        if (alt) paste = !paste;
        if (shift) { paste = true; plain = !plain; }
        return {paste, plain};
    }

    void PasteItem(int index) {
        if (m_loadingList || m_pasting || index < 0 || static_cast<size_t>(index) >= m_items.size()) {
            return;
        }
        const HWND target = m_targetWindow;
        const HWND target_focus = m_targetFocusWindow;
        const sqlite3_int64 id = m_items[static_cast<size_t>(index)].id;
        try {
            const auto [paste, plain] = ResolvePasteAction(m_settings.paste_by_default,
                m_settings.remove_formatting_by_default, (GetKeyState(VK_MENU) & 0x8000) != 0,
                (GetKeyState(VK_SHIFT) & 0x8000) != 0);
            const auto item = m_database.GetItem(id, true);
            if (!item || !SetClipboardItem(*item, plain)) {
                return;
            }
            m_pasting = true;
            // EmptyClipboard/SetClipboardData posts WM_CLIPBOARDUPDATE.  Do
            // not immediately record the item that the user just selected.
            m_skipNextClipboardEvent = true;
            HideMainWindow();
            m_database.MarkCopied(id);
            RestoreTargetFocusAndPaste(target, target_focus, paste);
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

    void ClearHistory(bool all = false) {
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
            if (FAILED(result)) button = MessageBoxW(config.pszMainInstruction, config.pszWindowTitle, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING);
            m_modalShowing = false;
            if (button != IDYES) return;
            if (checked) m_database.SetSetting(L"behavior.suppressClearAlert", L"1");
        }
        try {
            if (all) m_database.DeleteAll(); else m_database.DeleteUnpinned();
            if (m_settings.clear_system_clipboard && ::OpenClipboard(m_hWnd)) { EmptyClipboard(); CloseClipboard(); }
            ::SetWindowTextW(m_search, L"");
            RefreshHistory(L"");
        } catch (const std::exception &error) {
            MessageBoxA(m_hWnd, error.what(), "Unable to clear history", MB_OK | MB_ICONERROR);
        }
    }

    void OpenAbout() {
        m_modalShowing = true;
        MessageBoxW(L"Clipboard 1.0\n\n轻量 Windows 剪贴板历史工具\n布局和交互参考 Maccy 2.7.1\n\n使用 C++、WTL 和 SQLite 构建。", L"关于 Clipboard", MB_OK | MB_ICONINFORMATION);
        m_modalShowing = false;
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

    void ApplySettings() {
        const AppSettings previous = m_settings;
        m_settings = AppSettings::Load(m_database);
        ReloadIgnoreLists();
        m_previewTip = L"显示或隐藏预览（" + HotKeyToText(m_settings.preview_hotkey) + L"）";
        TOOLINFOW info{sizeof(info)}; info.uFlags = TTF_IDISHWND; info.hwnd = m_hWnd;
        info.uId = reinterpret_cast<UINT_PTR>(m_previewToggle); info.lpszText = m_previewTip.data();
        SendMessageW(m_tooltips, TTM_UPDATETIPTEXTW, 0, reinterpret_cast<LPARAM>(&info));
        if (previous.preview_width != m_settings.preview_width) {
            m_previewWindow.SetWidth(m_settings.preview_width);
        }
        if (m_popupVisible &&
            (previous.window_width != m_settings.window_width ||
             previous.window_height != m_settings.window_height)) {
            RECT rect{};
            if (::GetWindowRect(m_hWnd, &rect)) {
                ::SetWindowPos(
                    m_hWnd,
                    HWND_TOPMOST,
                    rect.left,
                    rect.top,
                    PopupWidth(),
                    PopupHeight(),
                    SWP_NOACTIVATE
                );
            }
        }
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
        ScheduleSearch();
        LayoutHistoryControls();
    }

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        CreateFonts();
        if (!BindControls() || !m_previewWindow.Initialize(m_hWnd, m_settings.preview_width)) {
            handled = FALSE;
            return FALSE;
        }
        ReloadIgnoreLists();
        m_clipboardListenerAdded = !m_isolated && AddClipboardFormatListener(m_hWnd) == TRUE;
        m_hotkeyRegistered = !m_isolated && RegisterHotKey(
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

    LRESULT OnMove(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        if (m_previewWindow.IsVisible()) {
            PositionPreviewWindow();
        }
        return 0;
    }

    LRESULT OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        m_inSizeMove = false;
        SaveWindowGeometry(true);
        if (m_previewWindow.IsVisible()) {
            PositionPreviewWindow();
        }
        return 0;
    }

    LRESULT OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL &handled) {
        m_inSizeMove = true; handled = TRUE; return 0;
    }

    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
        auto *info = reinterpret_cast<MINMAXINFO *>(lParam);
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

    LRESULT OnNcHitTest(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
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
        if (top && left) {
            return HTTOPLEFT;
        }
        if (top && right) {
            return HTTOPRIGHT;
        }
        if (bottom && left) {
            return HTBOTTOMLEFT;
        }
        if (bottom && right) {
            return HTBOTTOMRIGHT;
        }
        if (left) {
            return HTLEFT;
        }
        if (right) {
            return HTRIGHT;
        }
        if (top) {
            return HTTOP;
        }
        if (bottom) {
            return HTBOTTOM;
        }
        // The popup has no caption. Empty content-area space behaves like a
        // caption so the user can move it without bringing back a title bar.
        return HTCAPTION;
    }

    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL &) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(&paint);
        RECT client{};
        GetClientRect(&client);
        FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));
        HPEN separator = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
        auto previousPen = SelectObject(dc, separator);
        for (int y : {m_pinSeparatorY, m_footerSeparatorY}) if (y >= 0) {
            MoveToEx(dc, 16, y, nullptr); LineTo(dc, client.right - 16, y);
        }
        SelectObject(dc, previousPen); DeleteObject(separator);
        if (::IsWindowVisible(m_search)) {
            auto pen = SelectObject(dc, GetStockObject(NULL_PEN));
            auto brush = SelectObject(dc, GetSysColorBrush(COLOR_BTNFACE));
            RoundRect(dc, m_searchRect.left, m_searchRect.top, m_searchRect.right, m_searchRect.bottom, 8, 8);
            SelectObject(dc, brush); SelectObject(dc, pen);
            HPEN iconPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
            pen = SelectObject(dc, iconPen); brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
            const int iconLeft = m_searchRect.left + 7;
            const int iconTop = m_searchRect.top + 7;
            Ellipse(dc, iconLeft, iconTop, iconLeft + 9, iconTop + 9);
            MoveToEx(dc, iconLeft + 7, iconTop + 7, nullptr);
            LineTo(dc, iconLeft + 13, iconTop + 13);
            SelectObject(dc, brush); SelectObject(dc, pen); DeleteObject(iconPen);
        }
        if (m_settings.show_title && !IsRectEmpty(&m_titleRect)) {
            const HFONT previous_font = static_cast<HFONT>(SelectObject(
                dc,
                m_smallFont != nullptr ? m_smallFont : m_normalFont
            ));
            const int previous_color = SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
            const int previous_mode = SetBkMode(dc, TRANSPARENT);
            RECT title = m_titleRect;
            DrawTextW(dc, L"Clipboard", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            SetBkMode(dc, previous_mode);
            SetTextColor(dc, previous_color);
            SelectObject(dc, previous_font);
        }
        EndPaint(&paint);
        return 0;
    }

    LRESULT OnEditColor(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled) {
        const bool title = ::GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == IDC_HISTORY_TITLE;
        handled = reinterpret_cast<HWND>(lParam) == m_search || title;
        if (!handled) return 0;
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, GetSysColor(title ? COLOR_GRAYTEXT : COLOR_WINDOWTEXT));
        SetBkColor(dc, GetSysColor(title ? COLOR_WINDOW : COLOR_BTNFACE));
        return reinterpret_cast<LRESULT>(GetSysColorBrush(title ? COLOR_WINDOW : COLOR_BTNFACE));
    }

    LRESULT OnEraseBackground(UINT, WPARAM, LPARAM, BOOL &) {
        return 1;
    }

    LRESULT OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
        auto *measure = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
        if (measure == nullptr || (measure->CtlID != kHistoryListControlId && measure->CtlID != IDC_HISTORY_PINS)) {
            handled = FALSE;
            return 0;
        }
        handled = TRUE;
        // Keep every history entry at a single text line. Images and files
        // are represented by compact markers; their full content belongs in
        // the independent preview window.
        measure->itemHeight = kHistoryItemHeight;
        return 0;
    }

    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
        auto *draw = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
        if (draw && draw->CtlType == ODT_BUTTON) { DrawMenuButton(draw); handled = TRUE; return 0; }
        if (draw == nullptr || (draw->CtlID != kHistoryListControlId && draw->CtlID != IDC_HISTORY_PINS)) {
            handled = FALSE;
            return 0;
        }
        handled = TRUE;
        DrawHistoryItem(draw);
        return 0;
    }

    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
        if (LOWORD(wParam) == WA_INACTIVE && !m_modalShowing) {
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

    LRESULT OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled) {
        const UINT command = LOWORD(wParam);
        const UINT notification = HIWORD(wParam);
        if (notification == BN_CLICKED && command >= IDC_PREVIEW_PIN && command <= IDC_PREVIEW_CLOSE) {
            handled = TRUE;
            if (command == IDC_PREVIEW_CLOSE) { m_previewSuppressed = true; HidePreview(); FocusSearchOrPopup(); }
            else {
                SetActiveHistoryItem(m_activeItemIndex);
                if (command == IDC_PREVIEW_PIN) ToggleSelectedPin(); else DeleteSelectedItem();
            }
            return 0;
        }
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
            UpdateFooterControls();
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
            handled = TRUE; OpenAbout(); return 0;
        }
        if (notification == BN_CLICKED && command == IDC_HISTORY_SEARCH_CLEAR) {
            handled = TRUE; ::SetWindowTextW(m_search, L""); RefreshHistory(L""); FocusSearchOrPopup(); return 0;
        }
        if (notification == BN_CLICKED && command == IDC_HISTORY_PREVIEW) {
            handled = TRUE; TogglePreview(); FocusSearchOrPopup(); return 0;
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
            if (!m_loadingList && !m_mouseSelectionUpdate) {
                const HWND list = reinterpret_cast<HWND>(lParam);
                const LRESULT selected = ItemIndex(list, static_cast<int>(SendMessageW(list, LB_GETCURSEL, 0, 0)));
                if (selected != LB_ERR && static_cast<size_t>(selected) < m_items.size()) {
                    const int selected_index = static_cast<int>(selected);
                    const bool selected_by_mouse = m_hoveredItemIndex == selected_index;
                    SetActiveHistoryItem(selected_index);
                    if (!selected_by_mouse) {
                        m_hoveredItemIndex = -1;
                        m_hoveredItemId = 0;
                        m_previewSource = PreviewSource::Keyboard;
                        if (m_previewWindow.IsVisible()) ShowPreviewForItem(m_activeItemId);
                        else SchedulePreviewForItem(m_activeItemId, PreviewSource::Keyboard);
                    }
                }
            }
            return 0;
        }
        handled = FALSE;
        return 0;
    }

    LRESULT OnTimer(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        if (wParam == kPasteTimerId) {
            handled = TRUE;
            if (GetForegroundWindow() != m_pendingPasteTarget || GetTickCount64() > m_pendingPasteDeadline) {
                KillTimer(kPasteTimerId); m_pendingPasteTarget = nullptr;
            } else if (!PasteModifiersDown()) {
                KillTimer(kPasteTimerId);
                const HWND target = m_pendingPasteTarget;
                m_pendingPasteTarget = nullptr;
                RestoreTargetFocusAndPaste(target, m_pendingPasteFocus, true);
            }
            return 0;
        }
        if (wParam == kSearchTimerId) {
            handled = TRUE;
            KillTimer(kSearchTimerId);
            RefreshHistory(ReadWindowText(m_search));
            return 0;
        }
        if (wParam == kPreviewTimerId) {
            handled = TRUE;
            KillTimer(kPreviewTimerId);
            ShowPreviewForCandidate();
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
                    UpdateFooterControls();
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
        handled = !IsComposing(m_hWnd) && HandlePopupKey(wParam);
        return 0;
    }

    LRESULT OnImeStart(UINT, WPARAM, LPARAM, BOOL &handled) {
        m_imeComposing = true; handled = FALSE; return 0;
    }

    LRESULT OnImeEnd(UINT, WPARAM, LPARAM, BOOL &handled) {
        m_imeComposing = false; handled = FALSE; return 0;
    }

    LRESULT OnKeyUp(UINT, WPARAM, LPARAM, BOOL &handled) {
        UpdateFooterControls(); handled = FALSE; return 0;
    }

    LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
        handled = m_settings.show_search && wParam >= 0x20 && wParam != 0x7f;
        if (handled) TypeToSearch(wParam);
        return 0;
    }

    LRESULT OnTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL &) {
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
            } else ShowMainWindow();
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
    HWND m_pinsList = nullptr;
    HWND m_hoverList = nullptr;
    HWND m_searchClear = nullptr;
    HWND m_previewToggle = nullptr;
    HWND m_tooltips = nullptr;
    std::wstring m_previewTip;
    WNDPROC m_originalPinsProc = nullptr;
    int m_activeFooter = -1;
    bool m_imeComposing = false;
    bool m_previewSuppressed = false;
    bool m_modalShowing = false;
    RECT m_searchRect{};
    int m_pinSeparatorY = -1;
    int m_footerSeparatorY = -1;
    HWND m_footerClear = nullptr;
    HWND m_footerSettings = nullptr;
    HWND m_footerAbout = nullptr;
    HWND m_footerExit = nullptr;
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
    HWND m_pendingPasteTarget = nullptr;
    HWND m_pendingPasteFocus = nullptr;
    ULONGLONG m_pendingPasteDeadline = 0;
    HWND m_targetFocusWindow = nullptr;
    RECT m_targetCaretRect{};
    HFONT m_normalFont = nullptr;
    HFONT m_smallFont = nullptr;
    HFONT m_boldFont = nullptr;
    HFONT m_italicFont = nullptr;
    HFONT m_underlineFont = nullptr;
    std::wstring m_searchQuery;
    std::wstring m_lastCopyText;
    int m_searchHeight = 0;
    RECT m_titleRect{};
    int m_activeItemIndex = -1;
    sqlite3_int64 m_activeItemId = 0;
    int m_hoveredItemIndex = -1;
    sqlite3_int64 m_hoveredItemId = 0;
    sqlite3_int64 m_previewCandidateId = 0;
    sqlite3_int64 m_previewItemId = 0;
    PreviewSource m_previewSource = PreviewSource::None;
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
    bool m_isolated = false;
    bool m_inSizeMove = false;
    bool m_keyboardNavigating = false;
    POINT m_keyboardPointer{};
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
