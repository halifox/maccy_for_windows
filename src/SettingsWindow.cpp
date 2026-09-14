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
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace {

enum SettingsControlId : int {
    kTabs = IDC_SETTINGS_TABS,

    kGLaunch = IDC_G_LAUNCH,
    kGOpenHotKey,
    kGPinHotKey,
    kGDeleteHotKey,
    kGPreviewHotKey,
    kGSearchMode,
    kGPasteByDefault,
    kGRemoveFormatting,

    kAPopupPosition = IDC_A_POPUP_POSITION,
    kAPopupScreen = IDC_A_POPUP_SCREEN,
    kAResetPosition = IDC_A_RESET_POSITION,
    kAPinTo = IDC_A_PIN_TO,
    kAImageHeight = IDC_A_IMAGE_HEIGHT,
    kAOpenPreview = IDC_A_OPEN_PREVIEW,
    kAPreviewDelay = IDC_A_PREVIEW_DELAY,
    kAHighlight = IDC_A_HIGHLIGHT,
    kAMenuIcon = IDC_A_MENU_ICON,
    kAShowStatus = IDC_A_SHOW_STATUS,
    kAShowRecent = IDC_A_SHOW_RECENT,
    kAShowSearch = IDC_A_SHOW_SEARCH,
    kASearchVisibility = IDC_A_SEARCH_VISIBILITY,
    kAShowTitle = IDC_A_SHOW_TITLE,
    kAShowFooter = IDC_A_SHOW_FOOTER,
    kAShowSpecial = IDC_A_SHOW_SPECIAL,
    kAShowIcons = IDC_A_SHOW_ICONS,
    kAShowSwatch = IDC_A_SHOW_SWATCH,

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
// The resource dimensions are expressed in dialog units and provide the
// compact default size. These scaled pixel values are only a lower bound for
// unusual font metrics or DPI settings.
constexpr int kMinimumSettingsWidth = 456;
constexpr int kMinimumSettingsHeight = 320;

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


namespace {

constexpr std::array<UINT, 6> kPageResources = {
    IDD_PAGE_GENERAL,
    IDD_PAGE_APPEARANCE,
    IDD_PAGE_STORAGE,
    IDD_PAGE_IGNORE,
    IDD_PAGE_PINS,
    IDD_PAGE_ADVANCED,
};

constexpr std::array<UINT, 3> kIgnorePageResources = {
    IDD_IGNORE_APPLICATIONS,
    IDD_IGNORE_FORMATS,
    IDD_IGNORE_REGEXPS,
};

INT_PTR CALLBACK ResourcePageDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_INITDIALOG) {
        auto *owner = reinterpret_cast<SettingsWindow *>(lParam);
        ::SetWindowLongPtrW(window, DWLP_USER, reinterpret_cast<LONG_PTR>(owner));
        return TRUE;
    }

    auto *owner = reinterpret_cast<SettingsWindow *>(
        ::GetWindowLongPtrW(window, DWLP_USER)
    );
    if (owner != nullptr && ::IsWindow(owner->Window()) &&
        (message == WM_COMMAND || message == WM_NOTIFY)) {
        ::SendMessageW(owner->Window(), message, wParam, lParam);
        return TRUE;
    }

    return FALSE;
}

HWND CreateResourcePage(UINT resource_id, HWND parent, SettingsWindow *owner) {
    return ::CreateDialogParamW(
        ::GetModuleHandleW(nullptr),
        MAKEINTRESOURCEW(resource_id),
        parent,
        ResourcePageDialogProc,
        reinterpret_cast<LPARAM>(owner)
    );
}

void ApplySectionFont(HWND page, HFONT font, std::initializer_list<int> ids) {
    if (page == nullptr || font == nullptr) {
        return;
    }
    for (const int id : ids) {
        ::SendMessageW(
            ::GetDlgItem(page, id),
            WM_SETFONT,
            reinterpret_cast<WPARAM>(font),
            TRUE
        );
    }
}

} // namespace

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

    for (size_t page = 0; page < kPageResources.size(); ++page) {
        m_pages[page] = CreateResourcePage(
            kPageResources[page],
            m_tabs.m_hWnd,
            this
        );
        if (m_pages[page] == nullptr) {
            return false;
        }
    }

    m_ignoreTabWindow = ::GetDlgItem(m_pages[kPageIgnore], kIgnoreTabs);
    if (m_ignoreTabWindow == nullptr) {
        return false;
    }
    m_ignoreTabs = m_ignoreTabWindow;

    const std::array<const wchar_t *, kIgnorePageCount> ignore_names = {
        L"应用程序", L"剪贴板格式", L"正则表达式"
    };
    for (const wchar_t *name : ignore_names) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(name);
        m_ignoreTabs.InsertItem(m_ignoreTabs.GetItemCount(), &item);
    }

    for (size_t page = 0; page < kIgnorePageResources.size(); ++page) {
        m_ignorePages[page] = CreateResourcePage(
            kIgnorePageResources[page],
            m_ignoreTabs.m_hWnd,
            this
        );
        if (m_ignorePages[page] == nullptr) {
            return false;
        }
    }

    PositionPages();
    return true;
}

void SettingsWindow::BindControls() {
    const auto get = [this](int page, int id) {
        return ::GetDlgItem(m_pages[static_cast<size_t>(page)], id);
    };

    m_gLaunch = get(kPageGeneral, kGLaunch);
    m_gOpenHotKey = get(kPageGeneral, kGOpenHotKey);
    m_gPinHotKey = get(kPageGeneral, kGPinHotKey);
    m_gDeleteHotKey = get(kPageGeneral, kGDeleteHotKey);
    m_gPreviewHotKey = get(kPageGeneral, kGPreviewHotKey);
    m_gSearchMode = get(kPageGeneral, kGSearchMode);
    m_gPasteByDefault = get(kPageGeneral, kGPasteByDefault);
    m_gRemoveFormatting = get(kPageGeneral, kGRemoveFormatting);

    m_aPopupPosition = get(kPageAppearance, kAPopupPosition);
    m_aPopupScreen = get(kPageAppearance, kAPopupScreen);
    m_aPinTo = get(kPageAppearance, kAPinTo);
    m_aImageHeight = get(kPageAppearance, kAImageHeight);
    m_aOpenPreview = get(kPageAppearance, kAOpenPreview);
    m_aPreviewDelay = get(kPageAppearance, kAPreviewDelay);
    m_aHighlight = get(kPageAppearance, kAHighlight);
    m_aMenuIcon = get(kPageAppearance, kAMenuIcon);
    m_aShowStatus = get(kPageAppearance, kAShowStatus);
    m_aShowRecent = get(kPageAppearance, kAShowRecent);
    m_aShowSearch = get(kPageAppearance, kAShowSearch);
    m_aSearchVisibility = get(kPageAppearance, kASearchVisibility);
    m_aShowTitle = get(kPageAppearance, kAShowTitle);
    m_aShowFooter = get(kPageAppearance, kAShowFooter);
    m_aShowSpecial = get(kPageAppearance, kAShowSpecial);
    m_aShowIcons = get(kPageAppearance, kAShowIcons);
    m_aShowSwatch = get(kPageAppearance, kAShowSwatch);

    m_sSaveFiles = get(kPageStorage, kSSaveFiles);
    m_sSaveImages = get(kPageStorage, kSSaveImages);
    m_sSaveText = get(kPageStorage, kSSaveText);
    m_sHistorySize = get(kPageStorage, kSHistorySize);
    m_sSortBy = get(kPageStorage, kSSortBy);
    m_sStorageSize = get(kPageStorage, IDC_S_STORAGE_SIZE);

    m_iWhitelist = ::GetDlgItem(m_ignorePages[0], kIWhitelist);

    m_pList = get(kPagePins, kPList);
    m_pKey = get(kPagePins, kPKey);
    m_pTitle = get(kPagePins, kPTitle);
    m_pContent = get(kPagePins, kPContent);
    m_pContentHint = get(kPagePins, IDC_P_CONTENT_HINT);

    m_xIgnoreEvents = get(kPageAdvanced, kXIgnoreEvents);
    m_xIgnoreNext = get(kPageAdvanced, kXIgnoreNext);
    m_xClearOnQuit = get(kPageAdvanced, kXClearOnQuit);
    m_xClearClipboard = get(kPageAdvanced, kXClearClipboard);

    ApplySectionFont(
        m_pages[kPageGeneral],
        m_sectionFont,
        {IDC_G_SECTION_STARTUP, IDC_G_SECTION_HOTKEYS, IDC_G_SECTION_BEHAVIOR}
    );
    ApplySectionFont(
        m_pages[kPageAppearance],
        m_sectionFont,
        {IDC_A_SECTION_POPUP, IDC_A_SECTION_PREVIEW, IDC_A_SECTION_SEARCH}
    );
    ApplySectionFont(
        m_pages[kPageStorage],
        m_sectionFont,
        {IDC_S_SECTION_TYPES, IDC_S_SECTION_HISTORY}
    );
    ApplySectionFont(m_pages[kPageIgnore], m_sectionFont, {IDC_I_SECTION_RULES});
    ApplySectionFont(m_pages[kPagePins], m_sectionFont, {IDC_P_SECTION_PINS});
    ApplySectionFont(m_pages[kPageAdvanced], m_sectionFont, {IDC_X_SECTION_CLIPBOARD});

    AddComboItem(m_gSearchMode, L"精确（不区分大小写）");
    AddComboItem(m_gSearchMode, L"模糊");
    AddComboItem(m_gSearchMode, L"正则表达式");
    AddComboItem(m_gSearchMode, L"混合（精确→正则→模糊）");

    AddComboItem(m_aPopupPosition, L"光标附近");
    AddComboItem(m_aPopupPosition, L"托盘图标附近");
    AddComboItem(m_aPopupPosition, L"目标窗口中心");
    AddComboItem(m_aPopupPosition, L"屏幕中心");
    AddComboItem(m_aPopupPosition, L"上次位置");

    AddComboItem(m_aPinTo, L"顶部");
    AddComboItem(m_aPinTo, L"底部");

    AddComboItem(m_aHighlight, L"颜色");
    AddComboItem(m_aHighlight, L"粗体");
    AddComboItem(m_aHighlight, L"斜体");
    AddComboItem(m_aHighlight, L"下划线");

    AddComboItem(m_aMenuIcon, L"Maccy");
    AddComboItem(m_aMenuIcon, L"剪贴板");
    AddComboItem(m_aMenuIcon, L"剪刀");
    AddComboItem(m_aMenuIcon, L"回形针");

    AddComboItem(m_aSearchVisibility, L"始终显示");
    AddComboItem(m_aSearchVisibility, L"搜索时显示");

    AddComboItem(m_sSortBy, L"最近复制时间");
    AddComboItem(m_sSortBy, L"首次复制时间");
    AddComboItem(m_sSortBy, L"复制次数");
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

RECT SettingsWindow::IgnorePageRect() const {
    RECT page{};
    if (m_ignoreTabs.m_hWnd == nullptr) {
        return page;
    }
    ::GetClientRect(m_ignoreTabs.m_hWnd, &page);
    TabCtrl_AdjustRect(m_ignoreTabs.m_hWnd, FALSE, &page);
    page.right = std::max(page.left, page.right);
    page.bottom = std::max(page.top, page.bottom);
    return page;
}

void SettingsWindow::PositionPages() {
    if (m_tabs.m_hWnd == nullptr) {
        return;
    }

    const RECT page = PageRect();
    const int width = std::max(0L, page.right - page.left);
    const int height = std::max(0L, page.bottom - page.top);

    for (size_t index = 0; index < m_pages.size(); ++index) {
        if (m_pages[index] != nullptr) {
            ::SetWindowPos(
                m_pages[index],
                nullptr,
                page.left,
                page.top,
                width,
                height,
                SWP_NOZORDER | SWP_NOACTIVATE
            );
            ::ShowWindow(
                m_pages[index],
                static_cast<int>(index) == m_currentPage ? SW_SHOW : SW_HIDE
            );
        }
    }

    const RECT ignore_page = IgnorePageRect();
    const int ignore_width = std::max(0L, ignore_page.right - ignore_page.left);
    const int ignore_height = std::max(0L, ignore_page.bottom - ignore_page.top);
    for (size_t index = 0; index < m_ignorePages.size(); ++index) {
        if (m_ignorePages[index] != nullptr) {
            ::SetWindowPos(
                m_ignorePages[index],
                nullptr,
                ignore_page.left,
                ignore_page.top,
                ignore_width,
                ignore_height,
                SWP_NOZORDER | SWP_NOACTIVATE
            );
            ::ShowWindow(
                m_ignorePages[index],
                static_cast<int>(index) == m_ignorePage ? SW_SHOW : SW_HIDE
            );
        }
    }
}

void SettingsWindow::SetPage(int page) {
    m_currentPage = std::clamp(page, 0, kPageCount - 1);
    if (m_tabs.m_hWnd != nullptr) {
        m_tabs.SetCurSel(m_currentPage);
    }
    PositionPages();
}

void SettingsWindow::SetIgnorePage(int page) {
    m_ignorePage = std::clamp(page, 0, kIgnorePageCount - 1);
    if (m_ignoreTabs.m_hWnd != nullptr) {
        m_ignoreTabs.SetCurSel(m_ignorePage);
    }

    const HWND page_window = m_ignorePages[static_cast<size_t>(m_ignorePage)];
    m_iList = ::GetDlgItem(page_window, kIList);
    m_iEdit = ::GetDlgItem(page_window, kIEdit);
    m_iDescription = ::GetDlgItem(page_window, IDC_I_DESCRIPTION);

    if (m_ignorePage == 0) {
        ::SetWindowTextW(
            m_iDescription,
            L"忽略来自指定 Windows 应用程序的复制。建议优先使用剪贴板格式规则；列表中保存的是 exe 路径。\r\n开启白名单后，只记录列表中的应用。 "
        );
    } else if (m_ignorePage == 1) {
        ::SetWindowTextW(
            m_iDescription,
            L"忽略指定的 Windows 剪贴板格式。可填写标准名称（如 CF_UNICODETEXT、CF_HDROP）或注册格式名称。恢复默认将替换为内置忽略格式规则。"
        );
    } else {
        ::SetWindowTextW(
            m_iDescription,
            L"当 Unicode 文本匹配任意有效正则表达式时，不会记录该次复制。无效表达式会被安全忽略。"
        );
    }

    RefreshIgnoreList();
    PositionPages();
}
void SettingsWindow::LoadGeneralControls() {
    SetCheck(m_gLaunch, m_settings.launch_at_login);
    SetHotKeyControl(m_gOpenHotKey, m_settings.open_hotkey);
    SetHotKeyControl(m_gPinHotKey, m_settings.pin_hotkey);
    SetHotKeyControl(m_gDeleteHotKey, m_settings.delete_hotkey);
    SetHotKeyControl(m_gPreviewHotKey, m_settings.preview_hotkey);
    SelectCombo(m_gSearchMode, static_cast<int>(m_settings.search_mode));
    SetCheck(m_gPasteByDefault, m_settings.paste_by_default);
    SetCheck(m_gRemoveFormatting, m_settings.remove_formatting_by_default);
}

void SettingsWindow::LoadAppearanceControls() {
    ::EnableWindow(m_aImageHeight, FALSE);
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
    SetCheck(m_aShowTitle, m_settings.show_title);
    SetCheck(m_aShowFooter, m_settings.show_footer);
    SetCheck(m_aShowSpecial, m_settings.show_special_symbols);
    SetCheck(m_aShowIcons, m_settings.show_application_icons);
    SetCheck(m_aShowSwatch, m_settings.show_hex_color_swatch);
    ::EnableWindow(m_aPreviewDelay, m_settings.open_preview_automatically);
    UpdateDependencies();
}

void SettingsWindow::UpdateDependencies() {
    const std::wstring hint = std::wstring(L"Enter：") +
        (!IsChecked(m_gPasteByDefault) ? L"复制" : IsChecked(m_gRemoveFormatting) ? L"纯文本粘贴" : L"粘贴") +
        L"；Alt+Enter：切换复制/粘贴；Shift+Enter：切换粘贴格式";
    ::SetWindowTextW(::GetDlgItem(m_pages[kPageGeneral], IDC_G_BEHAVIOR_HINT), hint.c_str());
    ::EnableWindow(m_aSearchVisibility, IsChecked(m_aShowSearch));
    ::EnableWindow(m_aShowTitle, IsChecked(m_aShowSearch));
    ::EnableWindow(m_aPreviewDelay, IsChecked(m_aOpenPreview));
    ::EnableWindow(m_aMenuIcon, IsChecked(m_aShowStatus));
    ::EnableWindow(m_aShowRecent, IsChecked(m_aShowStatus));
    const auto position = static_cast<PopupPosition>(ComboSelection(m_aPopupPosition));
    ::EnableWindow(::GetDlgItem(m_pages[kPageAppearance], kAResetPosition), position == PopupPosition::LastPosition);
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
    m_originalPinContent = PinTextContent(item);
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
            m_settings.show_title = IsChecked(m_aShowTitle);
            m_settings.show_footer = IsChecked(m_aShowFooter);
            m_settings.show_special_symbols = IsChecked(m_aShowSpecial);
            m_settings.show_application_icons = IsChecked(m_aShowIcons);
            m_settings.show_hex_color_swatch = IsChecked(m_aShowSwatch);
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
        UpdateDependencies();
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
    std::wstring key = ReadWindowText(m_pKey);
    std::transform(key.begin(), key.end(), key.begin(), towlower);
    if (key.size() != 1 || key[0] < L'a' || key[0] > L'z' ||
        std::wstring_view(L"acfuqvxyz").find(key[0]) != std::wstring_view::npos ||
        towupper(key[0]) == m_settings.pin_hotkey.virtual_key ||
        towupper(key[0]) == m_settings.delete_hotkey.virtual_key ||
        towupper(key[0]) == m_settings.preview_hotkey.virtual_key ||
        std::any_of(m_pins.begin(), m_pins.end(), [&](const auto &item) {
            return item.id != m_selectedPinId && item.pin.size() == 1 && towlower(item.pin[0]) == key[0];
        })) {
        MessageBoxW(L"请选择未使用且不与搜索、编辑或已配置快捷键冲突的单个英文字母。", L"置顶快捷键", MB_OK | MB_ICONWARNING);
        return;
    }
    const std::wstring title = ReadWindowText(m_pTitle);
    try {
        if (m_selectedPinTextEditable && ReadWindowText(m_pContent) != m_originalPinContent) {
            if (MessageBoxW(L"修改内容将保存为纯文本并移除原有格式。继续？", L"修改置顶内容", MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES) return;
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
    MessageBoxW(L"此 Windows 版本尚未提供更新服务。", L"检查更新", MB_OK);
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
    // The settings window is deliberately a normal fixed-size top-level
    // window.  It has a title bar, participates in the taskbar and Alt+Tab,
    // and does not inherit the main window's topmost behavior.
    ::SetWindowTextW(m_hWnd, L"剪贴板设置");
    const LONG_PTR extended_style = ::GetWindowLongPtrW(m_hWnd, GWL_EXSTYLE);
    ::SetWindowLongPtrW(
        m_hWnd,
        GWL_EXSTYLE,
        (extended_style & ~WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW
    );
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
    BindControls();
    LoadControlsFromSettings();
    SetPage(kPageGeneral);
    SetIgnorePage(0);
    return TRUE;
}

LRESULT SettingsWindow::OnSize(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    PositionPages();
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
    PositionPages();
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
        id == kGLaunch || id == kGOpenHotKey || id == kGPinHotKey ||
        id == kGDeleteHotKey || id == kGPreviewHotKey || id == kGSearchMode ||
        id == kGPasteByDefault || id == kGRemoveFormatting;
    const bool appearance_change =
        id == kAPopupPosition || id == kAPopupScreen || id == kAPinTo || id == kAImageHeight ||
        id == kAOpenPreview || id == kAPreviewDelay || id == kAHighlight || id == kAMenuIcon ||
        id == kAShowStatus || id == kAShowRecent || id == kAShowSearch || id == kASearchVisibility ||
        id == kAShowTitle || id == kAShowFooter || id == kAShowSpecial || id == kAShowIcons || id == kAShowSwatch;
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
