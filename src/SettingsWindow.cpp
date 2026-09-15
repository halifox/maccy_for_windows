#include "SettingsWindow.h"
#include "Constants.h"

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "PinKeys.h"

// EditPinDialog 实现
EditPinDialog::EditPinDialog(Database &database, const AppSettings &settings, sqlite3_int64 item_id, const std::vector<ClipboardItem> &pins)
    : m_database(database), m_settings(settings), m_itemId(item_id), m_pins(pins) {
}

LRESULT EditPinDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    CenterWindow(GetParent());

    HWND keyCombo = GetDlgItem(IDC_EDIT_PIN_KEY);
    HWND titleEdit = GetDlgItem(IDC_EDIT_PIN_TITLE);
    HWND contentEdit = GetDlgItem(IDC_EDIT_PIN_CONTENT);
    HWND hintLabel = GetDlgItem(IDC_P_CONTENT_HINT);

    // 加载当前项目数据
    std::optional<ClipboardItem> itemOpt = m_database.GetItem(m_itemId);
    if (!itemOpt) {
        EndDialog(IDCANCEL);
        return TRUE;
    }

    ClipboardItem item = std::move(*itemOpt);
    m_key = item.pin;
    m_title = item.title;
    m_originalContent = item.content;

    // 填充键位下拉框
    for (wchar_t ch = L'a'; ch <= L'z'; ++ch) {
        std::wstring key(1, ch);
        ::SendMessageW(keyCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(key.c_str()));
    }
    for (wchar_t ch = L'0'; ch <= L'9'; ++ch) {
        std::wstring key(1, ch);
        ::SendMessageW(keyCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(key.c_str()));
    }

    // 设置当前键位
    int keyIndex = ::SendMessageW(keyCombo, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), reinterpret_cast<LPARAM>(m_key.c_str()));
    if (keyIndex != CB_ERR) {
        ::SendMessageW(keyCombo, CB_SETCURSEL, keyIndex, 0);
    }

    // 设置标题和内容
    ::SetWindowTextW(titleEdit, m_title.c_str());

    // 检查是否可编辑文本内容
    m_textEditable = item.has_text && !item.has_image && !item.has_files;
    if (m_textEditable) {
        ::SetWindowTextW(contentEdit, m_originalContent.c_str());
        ::EnableWindow(contentEdit, TRUE);
        ::SetWindowTextW(hintLabel, L"");
    } else {
        ::SetWindowTextW(contentEdit, m_originalContent.c_str());
        ::EnableWindow(contentEdit, FALSE);
        ::SetWindowTextW(hintLabel, L"此项目包含格式或非文本内容，无法在此编辑。");
    }

    return TRUE;
}

LRESULT EditPinDialog::OnOK(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;

    HWND keyCombo = GetDlgItem(IDC_EDIT_PIN_KEY);
    HWND titleEdit = GetDlgItem(IDC_EDIT_PIN_TITLE);
    HWND contentEdit = GetDlgItem(IDC_EDIT_PIN_CONTENT);

    // 读取键位
    int keyIndex = static_cast<int>(::SendMessageW(keyCombo, CB_GETCURSEL, 0, 0));
    if (keyIndex != CB_ERR) {
        wchar_t keyBuffer[256] = {};
        ::SendMessageW(keyCombo, CB_GETLBTEXT, keyIndex, reinterpret_cast<LPARAM>(keyBuffer));
        m_key = keyBuffer;
    }

    // 验证键位
    std::wstring keyLower = m_key;
    std::transform(keyLower.begin(), keyLower.end(), keyLower.begin(), std::towlower);
    if (!PinKeyPolicy::IsValid(keyLower, m_pins, m_settings, m_itemId)) {
        MessageBoxW(L"请选择未使用且不与搜索、编辑或已配置快捷键冲突的单个英文字母。", L"置顶快捷键", MB_OK | MB_ICONWARNING);
        return 0;
    }
    m_key = keyLower;

    // 读取标题
    int titleLen = ::GetWindowTextLengthW(titleEdit);
    if (titleLen > 0) {
        std::vector<wchar_t> buffer(titleLen + 1);
        ::GetWindowTextW(titleEdit, buffer.data(), titleLen + 1);
        m_title = buffer.data();
    } else {
        m_title.clear();
    }

    // 读取内容（如果可编辑）
    if (m_textEditable) {
        int contentLen = ::GetWindowTextLengthW(contentEdit);
        if (contentLen > 0) {
            std::vector<wchar_t> buffer(contentLen + 1);
            ::GetWindowTextW(contentEdit, buffer.data(), contentLen + 1);
            m_content = buffer.data();
        } else {
            m_content.clear();
        }

        if (m_content != m_originalContent) {
            if (MessageBoxW(L"修改内容将保存为纯文本并移除原有格式。继续？", L"修改置顶内容",
                MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES) {
                return 0;
            }
            m_contentModified = true;
        }
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT EditPinDialog::OnCancel(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;
    EndDialog(IDCANCEL);
    return 0;
}

// EditIgnoreDialog 实现
EditIgnoreDialog::EditIgnoreDialog(const std::wstring &value, const std::wstring &description, int ignore_page)
    : m_value(value), m_description(description), m_ignorePage(ignore_page) {
}

LRESULT EditIgnoreDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    CenterWindow(GetParent());

    HWND valueEdit = GetDlgItem(IDC_EDIT_IGNORE_VALUE);
    HWND descLabel = GetDlgItem(IDC_I_DESCRIPTION);

    ::SetWindowTextW(valueEdit, m_value.c_str());
    ::SetWindowTextW(descLabel, m_description.c_str());

    // 如果是新建，设置默认值
    if (m_value.empty()) {
        if (m_ignorePage == 1) {
            ::SetWindowTextW(valueEdit, L"xxx.yyy.zzz");
        } else if (m_ignorePage == 2) {
            ::SetWindowTextW(valueEdit, L"^[a-zA-Z0-9]{50}$");
        }
    }

    ::SendMessageW(valueEdit, EM_SETSEL, 0, -1);
    ::SetFocus(valueEdit);

    return FALSE; // 返回 FALSE 表示我们已经设置了焦点
}

LRESULT EditIgnoreDialog::OnOK(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;

    HWND valueEdit = GetDlgItem(IDC_EDIT_IGNORE_VALUE);
    int len = ::GetWindowTextLengthW(valueEdit);
    if (len > 0) {
        std::vector<wchar_t> buffer(len + 1);
        ::GetWindowTextW(valueEdit, buffer.data(), len + 1);
        m_value = buffer.data();
    } else {
        m_value.clear();
    }

    if (m_value.empty()) {
        MessageBoxW(L"值不能为空。", L"错误", MB_OK | MB_ICONWARNING);
        return 0;
    }

    EndDialog(IDOK);
    return 0;
}

LRESULT EditIgnoreDialog::OnCancel(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;
    EndDialog(IDCANCEL);
    return 0;
}

namespace {

enum SettingsControlId : int {
    kTabs = IDC_SETTINGS_TABS,

    kGLaunch = IDC_G_LAUNCH,
    kGUpdates = IDC_G_UPDATES,
    kGCheckNow = IDC_G_CHECK_NOW,
    kGOpenHotKey = IDC_G_OPEN_HOTKEY,
    kGPinHotKey = IDC_G_PIN_HOTKEY,
    kGDeleteHotKey = IDC_G_DELETE_HOTKEY,
    kGPreviewHotKey = IDC_G_PREVIEW_HOTKEY,
    kGSearchMode = IDC_G_SEARCH_MODE,
    kGPasteByDefault = IDC_G_PASTE_BY_DEFAULT,
    kGRemoveFormatting = IDC_G_REMOVE_FORMATTING,
    kGNotifications = IDC_G_NOTIFICATIONS,

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
    kSSaveImages = IDC_S_SAVE_IMAGES,
    kSSaveText = IDC_S_SAVE_TEXT,
    kSHistorySize = IDC_S_HISTORY_SIZE,
    kSSortBy = IDC_S_SORT_BY,
    kSStorageSize = IDC_S_STORAGE_SIZE,
    kSCurrentSize = IDC_S_CURRENT_SIZE,

    kIgnoreTabs = IDC_IGNORE_TABS,
    kIList = IDC_I_LIST,
    kIAdd = IDC_I_ADD,
    kIRemove = IDC_I_REMOVE,
    kIReset = IDC_I_RESET,
    kIWhitelist = IDC_I_WHITELIST,

    kPList = IDC_P_LIST,
    kPKey = IDC_P_KEY,
    kPTitle = IDC_P_TITLE,
    kPContent = IDC_P_CONTENT,
    kPSave = IDC_P_SAVE,
    kPDelete = IDC_P_DELETE,
    kPContentHint = IDC_P_CONTENT_HINT,

    kXIgnoreEvents = IDC_X_IGNORE_EVENTS,
    kXIgnoreNext = IDC_X_IGNORE_NEXT,
    kXClearOnQuit = IDC_X_CLEAR_ON_QUIT,
    kXClearClipboard = IDC_X_CLEAR_CLIPBOARD,
};

constexpr int kPageGeneral = 0;
constexpr int kPageStorage = 1;
constexpr int kPageAppearance = 2;
constexpr int kPagePins = 3;
constexpr int kPageIgnore = 4;
constexpr int kPageAdvanced = 5;
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

int SelectedListViewItem(HWND list) {
    return list == nullptr ? -1 : ListView_GetNextItem(list, -1, LVNI_SELECTED);
}

void MoveControl(HWND control, int x, int y, int width, int height) {
    if (control == nullptr) {
        return;
    }
    ::SetWindowPos(
        control,
        nullptr,
        x,
        y,
        std::max(width, 0),
        std::max(height, 0),
        SWP_NOZORDER | SWP_NOACTIVATE
    );
}

int DialogUnitWidth(HWND dialog, int units) {
    RECT rect{0, 0, units, 0};
    return ::MapDialogRect(dialog, &rect) ? rect.right : units;
}

int DialogUnitHeight(HWND dialog, int units) {
    RECT rect{0, 0, 0, units};
    return ::MapDialogRect(dialog, &rect) ? rect.bottom : units;
}

void ConfigureListView(HWND list, bool allow_label_editing, bool show_column_header) {
    if (list == nullptr) {
        return;
    }

    LONG_PTR style = ::GetWindowLongPtrW(list, GWL_STYLE);
    style |= LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
    if (allow_label_editing) {
        style |= LVS_EDITLABELS;
    } else {
        style &= ~static_cast<LONG_PTR>(LVS_EDITLABELS);
    }
    if (show_column_header) {
        style &= ~static_cast<LONG_PTR>(LVS_NOCOLUMNHEADER);
    } else {
        style |= LVS_NOCOLUMNHEADER;
    }
    ::SetWindowLongPtrW(list, GWL_STYLE, style);
    ::SetWindowPos(
        list,
        nullptr,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );
    ListView_SetExtendedListViewStyle(
        list,
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
    );
    while (ListView_DeleteColumn(list, 0)) {
    }
}

void AddListViewColumn(HWND list, int index, int width, const wchar_t *title) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.cx = width;
    column.pszText = const_cast<wchar_t *>(title);
    ListView_InsertColumn(list, index, &column);
}

void SetListViewColumnWidth(HWND list, int width) {
    if (list != nullptr) {
        ListView_SetColumnWidth(list, 0, std::max(width - 4, 0));
    }
}

struct PageDefinition {
    UINT resource_id;
    const wchar_t *title;
    int width;
    int height;
};

constexpr std::array<PageDefinition, 6> kPageDefinitions = {
    PageDefinition{IDD_PAGE_GENERAL, L"通用", 450, 300},
    PageDefinition{IDD_PAGE_STORAGE, L"存储", 450, 260},
    PageDefinition{IDD_PAGE_APPEARANCE, L"外观", 650, 300},
    PageDefinition{IDD_PAGE_PINS, L"置顶项", 500, 400},
    PageDefinition{IDD_PAGE_IGNORE, L"忽略", 500, 400},
    PageDefinition{IDD_PAGE_ADVANCED, L"高级", 450, 340},
};

struct IgnorePageDefinition {
    UINT resource_id;
    const wchar_t *title;
    DatabaseList database_list;
};

constexpr std::array<IgnorePageDefinition, 3> kIgnorePageDefinitions = {
    IgnorePageDefinition{IDD_IGNORE_APPLICATIONS, L"忽略应用", DatabaseList::IgnoredApplications},
    IgnorePageDefinition{IDD_IGNORE_FORMATS, L"忽略剪贴板类型", DatabaseList::IgnoredFormats},
    IgnorePageDefinition{IDD_IGNORE_REGEXPS, L"正则表达式", DatabaseList::IgnoredRegexps},
};

DatabaseList IgnoreListForPage(int page) {
    if (page >= 0 && page < static_cast<int>(kIgnorePageDefinitions.size())) {
        return kIgnorePageDefinitions[static_cast<size_t>(page)].database_list;
    }
    return DatabaseList::IgnoredApplications;
}

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

void SettingsWindow::CreateTabs() {
    m_tabs = GetDlgItem(kTabs);
    SetControlFont(m_tabs.m_hWnd);

    for (const PageDefinition &definition : kPageDefinitions) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(definition.title);
        m_tabs.InsertItem(m_tabs.GetItemCount(), &item);
    }
}

bool SettingsWindow::CreatePageWindows() {
    if (m_tabs.m_hWnd == nullptr) {
        return false;
    }

    for (size_t page = 0; page < kPageDefinitions.size(); ++page) {
        m_pages[page] = CreateResourcePage(
            kPageDefinitions[page].resource_id,
            m_tabs.m_hWnd,
            this
        );
        if (m_pages[page] == nullptr) {
            return false;
        }
    }

    const HWND ignore_tab_window = ::GetDlgItem(m_pages[kPageIgnore], kIgnoreTabs);
    if (ignore_tab_window == nullptr) {
        return false;
    }
    m_ignoreTabs = ignore_tab_window;

    for (const IgnorePageDefinition &definition : kIgnorePageDefinitions) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(definition.title);
        m_ignoreTabs.InsertItem(m_ignoreTabs.GetItemCount(), &item);
    }

    for (size_t page = 0; page < kIgnorePageDefinitions.size(); ++page) {
        m_ignorePages[page] = CreateResourcePage(
            kIgnorePageDefinitions[page].resource_id,
            m_ignoreTabs.m_hWnd,
            this
        );
        if (m_ignorePages[page] == nullptr) {
            return false;
        }
    }

    // 初始化独立的忽略页面对象
    m_ignorePageObjects[0] = std::make_unique<IgnoreApplicationsPage>();
    m_ignorePageObjects[1] = std::make_unique<IgnoreFormatsPage>();
    m_ignorePageObjects[2] = std::make_unique<IgnoreRegexpsPage>();

    for (size_t page = 0; page < m_ignorePageObjects.size(); ++page) {
        if (m_ignorePageObjects[page] != nullptr) {
            m_ignorePageObjects[page]->Initialize(m_ignorePages[page], m_database);
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
    m_gUpdates = get(kPageGeneral, kGUpdates);
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
    m_sStorageSize = get(kPageStorage, kSStorageSize);
    m_sCurrentSize = get(kPageStorage, kSCurrentSize);

    m_pList = get(kPagePins, kPList);

    m_xIgnoreEvents = get(kPageAdvanced, kXIgnoreEvents);
    m_xIgnoreNext = get(kPageAdvanced, kXIgnoreNext);
    m_xClearOnQuit = get(kPageAdvanced, kXClearOnQuit);
    m_xClearClipboard = get(kPageAdvanced, kXClearClipboard);

    AddComboItem(m_gSearchMode, L"精确");
    AddComboItem(m_gSearchMode, L"模糊");
    AddComboItem(m_gSearchMode, L"正则表达式");
    AddComboItem(m_gSearchMode, L"混合");

    AddComboItem(m_aPopupPosition, L"光标");
    AddComboItem(m_aPopupPosition, L"菜单栏图标");
    AddComboItem(m_aPopupPosition, L"窗口中心");
    AddComboItem(m_aPopupPosition, L"屏幕中央");
    AddComboItem(m_aPopupPosition, L"最后位置");

    AddComboItem(m_aPinTo, L"顶部");
    AddComboItem(m_aPinTo, L"底部");

    AddComboItem(m_aHighlight, L"颜色");
    AddComboItem(m_aHighlight, L"粗体");
    AddComboItem(m_aHighlight, L"斜体");
    AddComboItem(m_aHighlight, L"强调");

    AddComboItem(m_aMenuIcon, L"Maccy");
    AddComboItem(m_aMenuIcon, L"剪贴板");
    AddComboItem(m_aMenuIcon, L"剪刀");
    AddComboItem(m_aMenuIcon, L"回形针");

    AddComboItem(m_aSearchVisibility, L"始终");
    AddComboItem(m_aSearchVisibility, L"在搜索过程中");

    AddComboItem(m_sSortBy, L"上次复制时间");
    AddComboItem(m_sSortBy, L"首次复制时间");
    AddComboItem(m_sSortBy, L"复制次数");

    ConfigureIgnoreList();
    ConfigurePinsList();
}

void SettingsWindow::ConfigureIgnoreList() {
    const UINT dpi = WindowDpi(m_hWnd);
    const int icon_size = MulDiv(16, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
    if (m_ignoreImageList == nullptr) {
        m_ignoreImageList = ImageList_Create(
            icon_size,
            icon_size,
            ILC_COLOR32 | ILC_MASK,
            8,
            8
        );
    } else {
        ImageList_SetIconSize(m_ignoreImageList, icon_size, icon_size);
    }

    for (size_t index = 0; index < m_ignorePages.size(); ++index) {
        const HWND list = ::GetDlgItem(m_ignorePages[index], kIList);
        ConfigureListView(list, index != 0, false);
        AddListViewColumn(list, 0, 100, L"");
        if (index == 0 && m_ignoreImageList != nullptr) {
            ListView_SetImageList(list, m_ignoreImageList, LVSIL_SMALL);
        }
    }
}

void SettingsWindow::ConfigurePinsList() {
    ConfigureListView(m_pList, false, true);
    AddListViewColumn(m_pList, 0, 58, L"键位");
    AddListViewColumn(m_pList, 1, 150, L"别名");
    AddListViewColumn(m_pList, 2, 260, L"内容");
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

void SettingsWindow::LayoutFlexibleControls() {
    if (m_pList != nullptr) {
        const HWND page_window = m_pages[kPagePins];
        RECT page{};
        ::GetClientRect(page_window, &page);
        const int width = std::max(0L, page.right - page.left);
        const int height = std::max(0L, page.bottom - page.top);
        const int left_margin = DialogUnitWidth(page_window, 8);
        const int right_margin = DialogUnitWidth(page_window, 8);
        const int hint_height = DialogUnitHeight(page_window, 14);
        const int top_margin = DialogUnitHeight(page_window, 5);
        const int bottom_margin = DialogUnitHeight(page_window, 8);
        const int hint_y = std::max(top_margin, height - bottom_margin - hint_height);
        const int list_height = std::max(0, hint_y - DialogUnitHeight(page_window, 2) - top_margin);

        MoveControl(
            m_pList,
            left_margin,
            top_margin,
            width - left_margin - right_margin,
            list_height
        );

        MoveControl(
            ::GetDlgItem(page_window, IDC_P_LIST_HINT),
            left_margin,
            hint_y,
            width - left_margin - right_margin,
            hint_height
        );

        RECT list_client{};
        ::GetClientRect(m_pList, &list_client);
        const int key_width = DialogUnitWidth(page_window, 58);
        const int alias_width = DialogUnitWidth(page_window, 150);
        ListView_SetColumnWidth(m_pList, 0, key_width);
        ListView_SetColumnWidth(m_pList, 1, alias_width);
        ListView_SetColumnWidth(
            m_pList,
            2,
            std::max(0L, list_client.right - key_width - alias_width)
        );
    }

    for (size_t index = 0; index < m_ignorePages.size(); ++index) {
        const HWND page_window = m_ignorePages[index];
        const HWND list = ::GetDlgItem(page_window, kIList);
        if (page_window == nullptr || list == nullptr) {
            continue;
        }

        RECT page{};
        ::GetClientRect(page_window, &page);
        const int width = std::max(0L, page.right - page.left);
        const int height = std::max(0L, page.bottom - page.top);
        const int left_margin = DialogUnitWidth(page_window, 8);
        const int right_margin = DialogUnitWidth(page_window, 8);
        const int button_height = DialogUnitHeight(page_window, 14);
        const int description_height = DialogUnitHeight(page_window, 36);
        const int bottom_margin = DialogUnitHeight(page_window, 8);
        const int description_y = std::max(bottom_margin, height - description_height - bottom_margin);
        const int button_y = std::max(bottom_margin, description_y - button_height - DialogUnitHeight(page_window, 2));
        const int list_bottom = std::max(DialogUnitHeight(page_window, 80), button_y - DialogUnitHeight(page_window, 2));

        MoveControl(
            list,
            left_margin,
            DialogUnitHeight(page_window, 8),
            width - left_margin - right_margin,
            list_bottom - DialogUnitHeight(page_window, 8)
        );
        MoveControl(
            ::GetDlgItem(page_window, kIAdd),
            left_margin,
            button_y,
            DialogUnitWidth(page_window, 28),
            button_height
        );
        MoveControl(
            ::GetDlgItem(page_window, kIRemove),
            DialogUnitWidth(page_window, 40),
            button_y,
            DialogUnitWidth(page_window, 28),
            button_height
        );
        MoveControl(
            ::GetDlgItem(page_window, kIWhitelist),
            DialogUnitWidth(page_window, 78),
            button_y,
            width - DialogUnitWidth(page_window, 86),
            DialogUnitHeight(page_window, 14)
        );
        MoveControl(
            ::GetDlgItem(page_window, kIReset),
            width - DialogUnitWidth(page_window, 58),
            button_y,
            DialogUnitWidth(page_window, 50),
            button_height
        );
        MoveControl(
            ::GetDlgItem(page_window, IDC_I_DESCRIPTION),
            left_margin,
            description_y,
            width - left_margin - right_margin,
            description_height
        );
        SetListViewColumnWidth(list, width - left_margin - right_margin);
    }
}

void SettingsWindow::PositionPages() {
    if (m_tabs.m_hWnd == nullptr) {
        return;
    }

    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    MoveControl(m_tabs.m_hWnd, 0, 0, client.right, client.bottom);

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

    LayoutFlexibleControls();
}

void SettingsWindow::EnsureCurrentPageFits() {
    if (m_hWnd == nullptr || m_tabs.m_hWnd == nullptr) {
        return;
    }

    const RECT page = PageRect();
    const int page_width = std::max(0L, page.right - page.left);
    const int page_height = std::max(0L, page.bottom - page.top);
    const UINT dpi = WindowDpi(m_hWnd);
    const PageDefinition &definition = kPageDefinitions[static_cast<size_t>(m_currentPage)];
    const int required_page_width = MulDiv(
        definition.width,
        static_cast<int>(dpi),
        USER_DEFAULT_SCREEN_DPI
    );
    const int required_page_height = MulDiv(
        definition.height,
        static_cast<int>(dpi),
        USER_DEFAULT_SCREEN_DPI
    );
    if (page_width >= required_page_width && page_height >= required_page_height) {
        return;
    }

    RECT window{};
    ::GetWindowRect(m_hWnd, &window);
    const int current_width = window.right - window.left;
    const int current_height = window.bottom - window.top;
    const int width = std::max(
        current_width,
        current_width + required_page_width - page_width
    );
    const int height = std::max(
        current_height,
        current_height + required_page_height - page_height
    );
    ::SetWindowPos(
        m_hWnd,
        nullptr,
        window.left,
        window.top,
        width,
        height,
        SWP_NOZORDER | SWP_NOACTIVATE
    );
}

void SettingsWindow::SetPage(int page) {
    m_currentPage = std::clamp(page, 0, AppConstants::SettingsUI::kPageCount - 1);
    if (m_tabs.m_hWnd != nullptr) {
        m_tabs.SetCurSel(m_currentPage);
    }

    // 如果切换到忽略页面，需要初始化当前忽略子标签页
    if (m_currentPage == kPageIgnore) {
        SetIgnorePage(m_ignorePage);
    }

    PositionPages();
    EnsureCurrentPageFits();
}

void SettingsWindow::SetIgnorePage(int page) {
    m_ignorePage = std::clamp(page, 0, AppConstants::SettingsUI::kIgnorePageCount - 1);
    if (m_ignoreTabs.m_hWnd != nullptr) {
        m_ignoreTabs.SetCurSel(m_ignorePage);
    }

    // 隐藏所有子页面
    for (size_t i = 0; i < m_ignorePageObjects.size(); ++i) {
        if (m_ignorePageObjects[i] != nullptr) {
            m_ignorePageObjects[i]->Hide();
        }
    }

    // 显示当前子页面
    if (m_ignorePageObjects[m_ignorePage] != nullptr) {
        m_ignorePageObjects[m_ignorePage]->Show();
    }

    PositionPages();
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

    const sqlite3_int64 current_count = m_database.CountItems();
    const std::wstring current_size_text = L"（当前: " + std::to_wstring(current_count) + L" 项）";
    ::SetWindowTextW(m_sCurrentSize, current_size_text.c_str());
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

    // 加载 whitelist 复选框（从 IgnoreFormatsPage 获取）
    if (m_ignorePageObjects[1] != nullptr) {
        HWND whitelist = ::GetDlgItem(m_ignorePageObjects[1]->GetPageWindow(), IDC_I_WHITELIST);
        SetCheck(whitelist, m_settings.ignore_all_apps_except_listed);
    }

    m_loading = false;
}

void SettingsWindow::RefreshPinsList() {
    if (m_pList == nullptr) {
        return;
    }

    // 保存当前选中项的 ID
    sqlite3_int64 previous_selection = 0;
    const int current_selected = SelectedListViewItem(m_pList);
    if (current_selected >= 0 && current_selected < static_cast<int>(m_pins.size())) {
        previous_selection = m_pins[current_selected].id;
    }

    m_pins = m_database.GetPinnedItems();
    std::stable_sort(m_pins.begin(), m_pins.end(), [](const ClipboardItem &lhs, const ClipboardItem &rhs) {
        if (lhs.first_copied_at != rhs.first_copied_at) {
            return lhs.first_copied_at < rhs.first_copied_at;
        }
        return lhs.id < rhs.id;
    });

    ListView_DeleteAllItems(m_pList);
    for (size_t index = 0; index < m_pins.size(); ++index) {
        const ClipboardItem &item = m_pins[index];
        LVITEMW row{};
        row.mask = LVIF_TEXT | LVIF_PARAM;
        row.iItem = static_cast<int>(index);
        row.lParam = item.id;
        row.pszText = const_cast<wchar_t *>(item.pin.c_str());
        ListView_InsertItem(m_pList, &row);

        ListView_SetItemText(
            m_pList,
            static_cast<int>(index),
            1,
            const_cast<wchar_t *>(item.title.c_str())
        );
        std::wstring content;
        if (item.has_text) {
            content = PinTextContent(item);
            if (content.empty()) {
                content = item.content;
            }
        } else {
            content = L"不可编辑的内容（图像或文件）";
        }
        for (wchar_t &character : content) {
            if (character == L'\r' || character == L'\n') {
                character = L' ';
            }
        }
        if (content.size() > 120) {
            content.resize(120);
            content += L"…";
        }
        ListView_SetItemText(
            m_pList,
            static_cast<int>(index),
            2,
            content.data()
        );
    }

    // 恢复选中项
    int selected_index = -1;
    if (previous_selection != 0) {
        for (size_t index = 0; index < m_pins.size(); ++index) {
            if (m_pins[index].id == previous_selection) {
                selected_index = static_cast<int>(index);
                break;
            }
        }
    }
    if (selected_index < 0 && !m_pins.empty()) {
        selected_index = 0;
    }
    if (selected_index >= 0) {
        ListView_SetItemState(
            m_pList,
            selected_index,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED
        );
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
            // 从 IgnoreFormatsPage 获取 whitelist 复选框状态
            if (m_ignorePageObjects[1] != nullptr) {
                HWND whitelist = ::GetDlgItem(m_ignorePageObjects[1]->GetPageWindow(), IDC_I_WHITELIST);
                m_settings.ignore_all_apps_except_listed = IsChecked(whitelist);
            }
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
        SendMessageW(m_owner, AppConstants::kSettingsChangedMessage, 0, 0);
    }
}

void SettingsWindow::EditSelectedPin() {
    const int selected = SelectedListViewItem(m_pList);
    if (selected < 0 || selected >= static_cast<int>(m_pins.size())) {
        return;
    }

    const sqlite3_int64 itemId = m_pins[selected].id;
    EditPinDialog dialog(m_database, m_settings, itemId, m_pins);
    if (dialog.DoModal(m_hWnd) != IDOK) {
        return;
    }

    try {
        if (dialog.ContentModified()) {
            m_database.UpdatePinnedItem(itemId, dialog.GetKey(), dialog.GetTitle(), dialog.GetContent());
        } else {
            m_database.UpdatePinnedMetadata(itemId, dialog.GetKey(), dialog.GetTitle());
        }
        RefreshPinsList();
        NotifyOwner();
    } catch (const std::exception &error) {
        MessageBoxA(m_hWnd, error.what(), "Unable to save pinned item", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::DeleteSelectedPin() {
    const int selected = SelectedListViewItem(m_pList);
    if (selected < 0 || selected >= static_cast<int>(m_pins.size())) {
        return;
    }
    if (::MessageBoxW(m_hWnd, L"删除当前置顶项目？", L"确认", MB_YESNO | MB_ICONQUESTION) != IDYES) {
        return;
    }
    try {
        m_database.DeleteItem(m_pins[selected].id);
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
    // Keep preferences as a normal top-level window so it remains visible in
    // the taskbar and Alt+Tab without inheriting the main window's topmost state.
    ::SetWindowTextW(m_hWnd, L"偏好设置");
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
    CreateTabs();
    if (!CreatePageWindows()) {
        handled = FALSE;
        return FALSE;
    }
    BindControls();
    LoadControlsFromSettings();
    SetPage(kPageGeneral);
    SetIgnorePage(0);
    EnsureCurrentPageFits();
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
    ConfigureIgnoreList();
    ConfigurePinsList();
    if (m_currentPage == kPageIgnore && m_ignorePageObjects[m_ignorePage] != nullptr) {
        m_ignorePageObjects[m_ignorePage]->Refresh();
    }
    PositionPages();
    EnsureCurrentPageFits();
    return 0;
}

LRESULT SettingsWindow::OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    handled = TRUE;
    auto *limits = reinterpret_cast<MINMAXINFO *>(lParam);
    if (limits == nullptr) {
        return 0;
    }

    const UINT dpi = WindowDpi(m_hWnd);
    int minimum_width = MulDiv(kMinimumSettingsWidth, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
    int minimum_height = MulDiv(kMinimumSettingsHeight, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
    const RECT page = PageRect();
    RECT window{};
    ::GetWindowRect(m_hWnd, &window);
    const int page_width = std::max(0L, page.right - page.left);
    const int page_height = std::max(0L, page.bottom - page.top);
    if (page_width > 0 && page_height > 0) {
        const PageDefinition &definition = kPageDefinitions[static_cast<size_t>(m_currentPage)];
        const int required_width = MulDiv(definition.width, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
        const int required_height = MulDiv(definition.height, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
        const int window_width = static_cast<int>(window.right - window.left);
        const int window_height = static_cast<int>(window.bottom - window.top);
        minimum_width = std::max(minimum_width, window_width + required_width - page_width);
        minimum_height = std::max(minimum_height, window_height + required_height - page_height);
    }
    limits->ptMinTrackSize.x = minimum_width;
    limits->ptMinTrackSize.y = minimum_height;
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
        if (m_ignorePageObjects[m_ignorePage] != nullptr) {
            m_ignorePageObjects[m_ignorePage]->AddValue();
        }
        return 0;
    }
    if (id == kIRemove && notification == BN_CLICKED) {
        if (m_ignorePageObjects[m_ignorePage] != nullptr) {
            m_ignorePageObjects[m_ignorePage]->RemoveValue();
        }
        return 0;
    }
    if (id == kIReset && notification == BN_CLICKED) {
        if (m_ignorePageObjects[m_ignorePage] != nullptr) {
            m_ignorePageObjects[m_ignorePage]->ResetToDefaults();
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
    // 检查是否来自当前忽略页面的列表
    if (m_ignorePageObjects[m_ignorePage] != nullptr) {
        HWND currentIgnoreList = ::GetDlgItem(m_ignorePageObjects[m_ignorePage]->GetPageWindow(), IDC_I_LIST);
        if (header->hwndFrom == currentIgnoreList) {
            if (header->code == NM_DBLCLK) {
                m_ignorePageObjects[m_ignorePage]->EditValue();
                return 0;
            }
            if (header->code == LVN_KEYDOWN) {
                const auto *key = reinterpret_cast<const NMLVKEYDOWN *>(lParam);
                if (key != nullptr && key->wVKey == VK_DELETE) {
                    m_ignorePageObjects[m_ignorePage]->RemoveValue();
                    return 0;
                }
            }
        }
    }

    if (header->hwndFrom == m_pList) {
        if (header->code == NM_DBLCLK) {
            EditSelectedPin();
            return 0;
        }
        if (header->code == LVN_KEYDOWN) {
            const auto *key = reinterpret_cast<const NMLVKEYDOWN *>(lParam);
            if (key != nullptr && key->wVKey == VK_DELETE) {
                DeleteSelectedPin();
                return 0;
            }
        }
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
    if (m_ignoreImageList != nullptr) {
        ImageList_Destroy(m_ignoreImageList);
        m_ignoreImageList = nullptr;
    }
    m_hWnd = nullptr;
    return 0;
}
