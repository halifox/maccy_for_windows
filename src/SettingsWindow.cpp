#include "SettingsWindow.h"
#include "Constants.h"
#include "ClipboardRules.h"

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
#include <utility>

#include "PinKeys.h"
#include "TrayIcon.h"

namespace {

std::wstring ReadWindowText(CWindow window) {
    if (window.m_hWnd == nullptr) {
        return {};
    }
    const int length = window.GetWindowTextLength();
    if (length <= 0) {
        return {};
    }
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = window.GetWindowText(text.data(), length + 1);
    text.resize(static_cast<size_t>(std::max(copied, 0)));
    return text;
}

} // namespace

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

// EditPinDialog 实现
EditPinDialog::EditPinDialog(const AppSettings &settings, sqlite3_int64 item_id,
                             const std::vector<ClipboardItem> &pins, ClipboardItem item)
    : m_settings(settings), m_itemId(item_id), m_pins(pins), m_item(std::move(item)) {
}

LRESULT EditPinDialog::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    CenterWindow(GetParent());

    m_keyCombo = GetDlgItem(IDC_EDIT_PIN_KEY);
    m_titleEdit = GetDlgItem(IDC_EDIT_PIN_TITLE);
    m_contentEdit = GetDlgItem(IDC_EDIT_PIN_CONTENT);
    m_hintLabel = GetDlgItem(IDC_P_CONTENT_HINT);

    ClipboardItem &item = m_item;
    m_key = item.pin;
    m_title = item.title;
    m_originalContent = PinTextContent(item);
    if (m_originalContent.empty()) {
        m_originalContent = item.preview;
    }

    // 只展示策略允许且当前项目可以使用的键位。
    for (const wchar_t ch : PinKeyPolicy::Available(m_pins, m_settings, m_itemId)) {
        std::wstring key(1, ch);
        m_keyCombo.AddString(key.c_str());
    }
    if (m_key.size() == 1 &&
        m_keyCombo.FindStringExact(-1, m_key.c_str()) == CB_ERR) {
        m_keyCombo.AddString(m_key.c_str());
    }

    // 设置当前键位
    int keyIndex = m_keyCombo.FindStringExact(-1, m_key.c_str());
    if (keyIndex != CB_ERR) {
        m_keyCombo.SetCurSel(keyIndex);
    }

    // 设置标题和内容
    m_titleEdit.SetWindowText(m_title.c_str());

    // 检查是否可编辑文本内容
    m_textEditable = item.has_text && !item.has_image && !item.has_files;
    if (m_textEditable) {
        m_contentEdit.SetWindowText(m_originalContent.c_str());
        m_contentEdit.EnableWindow(TRUE);
        m_hintLabel.SetWindowText(L"");
    } else {
        m_contentEdit.SetWindowText(m_originalContent.c_str());
        m_contentEdit.EnableWindow(FALSE);
        m_hintLabel.SetWindowText(L"此项目包含格式或非文本内容，无法在此编辑。");
    }

    return TRUE;
}

LRESULT EditPinDialog::OnOK(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;

    // 读取键位
    int keyIndex = m_keyCombo.GetCurSel();
    if (keyIndex != CB_ERR) {
        wchar_t keyBuffer[256] = {};
        m_keyCombo.GetLBText(keyIndex, keyBuffer);
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
    m_title = ReadWindowText(m_titleEdit);

    // 读取内容（如果可编辑）
    if (m_textEditable) {
        m_content = ReadWindowText(m_contentEdit);

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

    m_valueEdit = GetDlgItem(IDC_EDIT_IGNORE_VALUE);
    m_descriptionLabel = GetDlgItem(IDC_I_DESCRIPTION);

    m_valueEdit.SetWindowText(m_value.c_str());
    m_descriptionLabel.SetWindowText(m_description.c_str());

    // 如果是新建，设置默认值
    if (m_value.empty()) {
        if (m_ignorePage == 1) {
            m_valueEdit.SetWindowText(L"xxx.yyy.zzz");
        } else if (m_ignorePage == 2) {
            m_valueEdit.SetWindowText(L"^[a-zA-Z0-9]{50}$");
        }
    }

    m_valueEdit.SetSel(0, -1);
    m_valueEdit.SetFocus();

    return FALSE; // 返回 FALSE 表示我们已经设置了焦点
}

LRESULT EditIgnoreDialog::OnOK(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;

    m_value = ReadWindowText(m_valueEdit);

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
    kXRespectWindowsClipboardHistory = IDC_X_RESPECT_WINDOWS_CLIPBOARD_HISTORY,
};

constexpr int kPageGeneral = 0;
constexpr int kPageStorage = 1;
constexpr int kPageAppearance = 2;
constexpr int kPagePins = 3;
constexpr int kPageIgnore = 4;
constexpr int kPageAdvanced = 5;

UINT WindowDpi(CWindow window) {
    if (window.m_hWnd != nullptr) {
        CClientDC dc(window);
        const int dpi = dc.GetDeviceCaps(LOGPIXELSX);
        if (dpi > 0) {
            return static_cast<UINT>(dpi);
        }
    }
    return USER_DEFAULT_SCREEN_DPI;
}

void UpdateBehaviorHint(CStatic window, bool paste_default, bool plain_default) {
    std::wstring hint = ReadWindowText(window);
    constexpr std::wstring_view marker = L"• 按住 ";
    constexpr std::wstring_view suffix = L" 选择项目";
    const std::array<std::wstring, 3> key_expressions = {
        paste_default ? L"Alt+Enter" : L"Enter",
        paste_default ? L"Enter" : L"Alt+Enter",
        plain_default
            ? (paste_default ? L"Enter" : L"Alt+Shift+Enter")
            : (paste_default ? L"Ctrl+Shift+Enter" : L"Alt+Shift+Enter"),
    };

    size_t search_position = 0;
    for (const std::wstring &key_expression : key_expressions) {
        const size_t marker_position = hint.find(marker, search_position);
        if (marker_position == std::wstring::npos) {
            return;
        }
        const size_t key_start = marker_position + marker.size();
        const size_t key_end = hint.find(suffix, key_start);
        if (key_end == std::wstring::npos) {
            return;
        }
        hint.replace(key_start, key_end - key_start, key_expression);
        search_position = key_start + key_expression.size();
    }

    window.SetWindowText(hint.c_str());
}

int ReadValidatedInteger(CEdit window, int fallback, int minimum, int maximum) {
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
        window.SetWindowText(std::to_wstring(value).c_str());
        return value;
    } catch (...) {
        window.SetWindowText(std::to_wstring(safe_fallback).c_str());
        return safe_fallback;
    }
}

bool IsChecked(CButton window) {
    return window.m_hWnd != nullptr && window.GetCheck() == BST_CHECKED;
}

int ComboSelection(CComboBox window, int fallback = 0) {
    const int result = window.m_hWnd == nullptr ? CB_ERR : window.GetCurSel();
    return result == CB_ERR ? fallback : result;
}

void AddComboItem(CComboBox combo, const wchar_t *text) {
    combo.AddString(text);
}

void SetControlFont(CWindow window) {
    if (window.m_hWnd != nullptr) {
        window.SetFont(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT)), TRUE);
    }
}

int SelectedListViewItem(CListViewCtrl list) {
    return list.m_hWnd == nullptr ? -1 : list.GetNextItem(-1, LVNI_SELECTED);
}

void ConfigureListView(CListViewCtrl list, bool allow_label_editing, bool show_column_header) {
    if (list.m_hWnd == nullptr) {
        return;
    }

    const DWORD oldStyle = list.GetStyle();
    DWORD style = oldStyle;
    style |= LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
    if (allow_label_editing) {
        style |= LVS_EDITLABELS;
    } else {
        style &= ~static_cast<DWORD>(LVS_EDITLABELS);
    }
    if (show_column_header) {
        style &= ~static_cast<DWORD>(LVS_NOCOLUMNHEADER);
    } else {
        style |= LVS_NOCOLUMNHEADER;
    }
    list.ModifyStyle(
        oldStyle & ~style,
        style & ~oldStyle,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );
    list.SetExtendedListViewStyle(
        LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER
    );
    while (list.DeleteColumn(0)) {
    }
}

void AddListViewColumn(CListViewCtrl list, int index, int width, const wchar_t *title) {
    LVCOLUMNW column{};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.cx = width;
    column.pszText = const_cast<wchar_t *>(title);
    list.InsertColumn(index, &column);
}

struct PageDefinition {
    const wchar_t *title;
};

constexpr std::array<PageDefinition, 6> kPageDefinitions = {
    PageDefinition{L"通用"},
    PageDefinition{L"存储"},
    PageDefinition{L"外观"},
    PageDefinition{L"置顶项"},
    PageDefinition{L"忽略"},
    PageDefinition{L"高级"},
};

struct IgnorePageDefinition {
    const wchar_t *title;
};

constexpr std::array<IgnorePageDefinition, 3> kIgnorePageDefinitions = {
    IgnorePageDefinition{L"忽略应用"},
    IgnorePageDefinition{L"忽略剪贴板类型"},
    IgnorePageDefinition{L"正则表达式"},
};

HotKeyConfig HotKeyFromControl(const CHotKeyCtrl &control) {
    WORD virtual_key = 0;
    WORD flags = 0;
    control.GetHotKey(virtual_key, flags);
    UINT modifiers = 0;
    if ((flags & HOTKEYF_CONTROL) != 0) modifiers |= MOD_CONTROL;
    if ((flags & HOTKEYF_ALT) != 0) modifiers |= MOD_ALT;
    if ((flags & HOTKEYF_SHIFT) != 0) modifiers |= MOD_SHIFT;
    return HotKeyConfig{modifiers, static_cast<BYTE>(virtual_key)};
}

void SetHotKeyControl(CHotKeyCtrl &control, const HotKeyConfig &hotkey) {
    WORD flags = 0;
    if ((hotkey.modifiers & MOD_CONTROL) != 0) flags |= HOTKEYF_CONTROL;
    if ((hotkey.modifiers & MOD_ALT) != 0) flags |= HOTKEYF_ALT;
    if ((hotkey.modifiers & MOD_SHIFT) != 0) flags |= HOTKEYF_SHIFT;
    control.SetHotKey(hotkey.virtual_key, flags);
}

}

SettingsWindow::SettingsWindow(
    StorageWorker &storage,
    HWND owner,
    AppSettings settings,
    std::array<std::vector<std::wstring>, 3> ignored_lists,
    SettingsChangedCallback on_changed,
    UpdateCheckCallback on_update_check
)
    : m_storage(storage),
      m_owner(owner),
      m_onChanged(std::move(on_changed)),
      m_onUpdateCheck(std::move(on_update_check)),
      m_settings(std::move(settings)),
      m_ignoredLists(std::move(ignored_lists)) {}

void SettingsWindow::SetSettingsSnapshot(const AppSettings &settings) {
    SetStateSnapshot(settings, m_ignoredLists);
}

void SettingsWindow::SetStateSnapshot(
    const AppSettings &settings,
    StorageWorker::IgnoreLists ignored_lists
) {
    m_settings = settings;
    m_ignoredLists = std::move(ignored_lists);
    for (size_t page = 0; page < m_ignorePageObjects.size(); ++page) {
        if (m_ignorePageObjects[page] != nullptr) {
            m_ignorePageObjects[page]->SetValues(m_ignoredLists[page]);
        }
    }
    if (m_hWnd != nullptr && IsWindow()) {
        LoadControlsFromSettings();
    }
}

void SettingsWindow::SetUpdateCheckBusy(bool busy) {
    m_updateCheckBusy = busy;
    if (m_gCheckNow.m_hWnd != nullptr && m_gCheckNow.IsWindow()) {
        m_gCheckNow.EnableWindow(busy ? FALSE : TRUE);
    }
}

bool SettingsWindow::CreateOrShow() {
    if (m_hWnd != nullptr && IsWindow()) {
        LoadControlsFromSettings();
        SetWindowPos(
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

    HMONITOR monitor = ::MonitorFromWindow(m_owner.m_hWnd, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        monitor = ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    RECT work_area{0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN)};
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor != nullptr && ::GetMonitorInfoW(monitor, &monitor_info)) {
        work_area = monitor_info.rcWork;
    }

    RECT window_rect{};
    GetWindowRect(&window_rect);
    const int window_width = window_rect.right - window_rect.left;
    const int window_height = window_rect.bottom - window_rect.top;
    const int x = work_area.left + ((work_area.right - work_area.left) - window_width) / 2;
    const int y = work_area.top + ((work_area.bottom - work_area.top) - window_height) / 2;
    SetWindowPos(HWND_NOTOPMOST, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE);
    ::SetForegroundWindow(window);
    return true;
}

void SettingsWindow::DestroyForOwner() {
    m_destroying = true;
    if (m_hWnd != nullptr && IsWindow()) {
        DestroyWindow();
    }
}

void SettingsWindow::CreateTabs() {
    m_tabs = GetDlgItem(kTabs);
    SetControlFont(m_tabs);

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
        if (m_pages[page]->CreatePage(m_tabs.m_hWnd, *this) == nullptr) {
            return false;
        }
    }

    const CWindow ignoreTabWindow = m_pages[kPageIgnore]->Control(kIgnoreTabs);
    if (ignoreTabWindow.m_hWnd == nullptr) {
        return false;
    }
    m_ignoreTabs = ignoreTabWindow;

    for (const IgnorePageDefinition &definition : kIgnorePageDefinitions) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(definition.title);
        m_ignoreTabs.InsertItem(m_ignoreTabs.GetItemCount(), &item);
    }

    for (size_t page = 0; page < kIgnorePageDefinitions.size(); ++page) {
        if (m_ignorePages[page]->CreatePage(m_ignoreTabs.m_hWnd, *this) == nullptr) {
            return false;
        }
    }

    // 初始化独立的忽略页面对象
    m_ignorePageObjects[0] = std::make_unique<IgnoreApplicationsPage>();
    m_ignorePageObjects[1] = std::make_unique<IgnoreFormatsPage>();
    m_ignorePageObjects[2] = std::make_unique<IgnoreRegexpsPage>();

    for (size_t page = 0; page < m_ignorePageObjects.size(); ++page) {
        if (m_ignorePageObjects[page] != nullptr) {
            m_ignorePageObjects[page]->Initialize(
                m_ignorePages[page]->Window(),
                m_storage,
                m_ignoredLists[page]
            );
        }
    }

    return true;
}

void SettingsWindow::BindControls() {
    const auto get = [this](int page, int id) {
        return m_pages[static_cast<size_t>(page)]->Control(id);
    };

    m_gCheckNow = get(kPageGeneral, kGCheckNow);
    m_gBehaviorHint = get(kPageGeneral, IDC_G_BEHAVIOR_HINT);
    m_gOpenHotKey = get(kPageGeneral, kGOpenHotKey);
    m_gPinHotKey = get(kPageGeneral, kGPinHotKey);
    m_gDeleteHotKey = get(kPageGeneral, kGDeleteHotKey);
    m_gPreviewHotKey = get(kPageGeneral, kGPreviewHotKey);
    m_gPasteByDefault = get(kPageGeneral, kGPasteByDefault);
    m_gRemoveFormatting = get(kPageGeneral, kGRemoveFormatting);

    m_aPopupPosition = get(kPageAppearance, kAPopupPosition);
    m_aResetPosition = get(kPageAppearance, kAResetPosition);
    m_aImageHeight = get(kPageAppearance, kAImageHeight);
    m_aOpenPreview = get(kPageAppearance, kAOpenPreview);
    m_aPreviewDelay = get(kPageAppearance, kAPreviewDelay);
    m_aMenuIcon = get(kPageAppearance, kAMenuIcon);
    m_aShowStatus = get(kPageAppearance, kAShowStatus);
    m_aShowSearch = get(kPageAppearance, kAShowSearch);
    m_aSearchVisibility = get(kPageAppearance, kASearchVisibility);
    m_aShowTitle = get(kPageAppearance, kAShowTitle);

    m_sHistorySize = get(kPageStorage, kSHistorySize);
    m_sStorageSize = get(kPageStorage, kSStorageSize);
    m_sCurrentSize = get(kPageStorage, kSCurrentSize);

    m_pList = get(kPagePins, kPList);
    m_xIgnoreEvents = get(kPageAdvanced, kXIgnoreEvents);
    m_xIgnoreNext = get(kPageAdvanced, kXIgnoreNext);

    CComboBox searchMode(get(kPageGeneral, kGSearchMode).m_hWnd);
    AddComboItem(searchMode, L"精确");
    AddComboItem(searchMode, L"模糊");
    AddComboItem(searchMode, L"正则表达式");
    AddComboItem(searchMode, L"混合");

    AddComboItem(m_aPopupPosition, L"光标");
    AddComboItem(m_aPopupPosition, L"菜单栏图标");
    AddComboItem(m_aPopupPosition, L"窗口中心");
    AddComboItem(m_aPopupPosition, L"屏幕中央");
    AddComboItem(m_aPopupPosition, L"最后位置");

    CComboBox pinTo(get(kPageAppearance, kAPinTo).m_hWnd);
    AddComboItem(pinTo, L"顶部");
    AddComboItem(pinTo, L"底部");

    CComboBox highlight(get(kPageAppearance, kAHighlight).m_hWnd);
    AddComboItem(highlight, L"颜色");
    AddComboItem(highlight, L"粗体");
    AddComboItem(highlight, L"斜体");
    AddComboItem(highlight, L"强调");

    AddComboItem(m_aMenuIcon, L"maccy");
    AddComboItem(m_aMenuIcon, L"剪贴板");
    AddComboItem(m_aMenuIcon, L"剪刀");
    AddComboItem(m_aMenuIcon, L"回形针");

    AddComboItem(m_aSearchVisibility, L"始终");
    AddComboItem(m_aSearchVisibility, L"在搜索过程中");

    CComboBox sortBy(get(kPageStorage, kSSortBy).m_hWnd);
    AddComboItem(sortBy, L"上次复制时间");
    AddComboItem(sortBy, L"首次复制时间");
    AddComboItem(sortBy, L"复制次数");

    ConfigureIgnoreList();
    ConfigurePinsList();
}

void SettingsWindow::ConfigureIgnoreList() {
    const UINT dpi = WindowDpi(CWindow(m_hWnd));
    const int icon_size = MulDiv(16, static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
    if (m_ignoreImageList.IsNull()) {
        m_ignoreImageList.Create(
            icon_size,
            icon_size,
            ILC_COLOR32 | ILC_MASK,
            8,
            8
        );
    } else {
        m_ignoreImageList.SetIconSize(icon_size, icon_size);
    }

    for (size_t index = 0; index < m_ignorePages.size(); ++index) {
        CListViewCtrl list;
        list = m_ignorePages[index]->Control(kIList);
        ConfigureListView(list, index != 0, false);
        AddListViewColumn(list, 0, 100, L"");
        if (index == 0 && !m_ignoreImageList.IsNull()) {
            list.SetImageList(m_ignoreImageList, LVSIL_SMALL);
        }
    }
}

void SettingsWindow::ConfigurePinsList() {
    ConfigureListView(m_pList, false, true);
    AddListViewColumn(m_pList, 0, 63, L"键位");
    AddListViewColumn(m_pList, 1, 158, L"别名");
    AddListViewColumn(m_pList, 2, 300, L"内容");
}

void SettingsWindow::SetPage(int page) {
    m_currentPage = std::clamp(page, 0, AppConstants::SettingsUI::kPageCount - 1);
    if (m_tabs.m_hWnd != nullptr) {
        m_tabs.SetCurSel(m_currentPage);
    }

    for (size_t index = 0; index < m_pages.size(); ++index) {
        CWindow pageWindow = m_pages[index]->Window();
        if (pageWindow.m_hWnd != nullptr) {
            pageWindow.ShowWindow(
                static_cast<int>(index) == m_currentPage ? SW_SHOW : SW_HIDE
            );
        }
    }

    // 如果切换到忽略页面，需要初始化当前忽略子标签页
    if (m_currentPage == kPageIgnore) {
        SetIgnorePage(m_ignorePage);
    }
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
}

void SettingsWindow::LoadGeneralControls() {
    auto &page = *m_pages[kPageGeneral];
    page.LoadSettings(m_settings);
    page.ExchangeSettings(DDX_LOAD);
    SetHotKeyControl(m_gOpenHotKey, m_settings.open_hotkey);
    SetHotKeyControl(m_gPinHotKey, m_settings.pin_hotkey);
    SetHotKeyControl(m_gDeleteHotKey, m_settings.delete_hotkey);
    SetHotKeyControl(m_gPreviewHotKey, m_settings.preview_hotkey);
}

void SettingsWindow::LoadAppearanceControls() {
    CComboBox popupScreen(m_pages[kPageAppearance]->Control(kAPopupScreen).m_hWnd);
    popupScreen.ResetContent();
    const int monitor_count = std::max(1, GetSystemMetrics(SM_CMONITORS));
    AddComboItem(popupScreen, L"活动屏幕");
    for (int index = 0; index < monitor_count; ++index) {
        const std::wstring name = L"显示器 " + std::to_wstring(index + 1);
        AddComboItem(popupScreen, name.c_str());
    }
    auto &page = *m_pages[kPageAppearance];
    page.LoadSettings(m_settings);
    page.ExchangeSettings(DDX_LOAD);
    m_aImageHeight.SetWindowText(std::to_wstring(m_settings.image_max_height).c_str());
    m_aPreviewDelay.SetWindowText(std::to_wstring(m_settings.preview_delay).c_str());
    m_aPreviewDelay.EnableWindow(m_settings.open_preview_automatically);
    UpdateDependencies();
}

void SettingsWindow::UpdateDependencies() {
    UpdateBehaviorHint(
        m_gBehaviorHint,
        IsChecked(m_gPasteByDefault),
        IsChecked(m_gRemoveFormatting)
    );
    m_aSearchVisibility.EnableWindow(IsChecked(m_aShowSearch));
    m_aShowTitle.EnableWindow(IsChecked(m_aShowSearch));
    m_aPreviewDelay.EnableWindow(IsChecked(m_aOpenPreview));
    m_aMenuIcon.EnableWindow(IsChecked(m_aShowStatus));
    const auto position = static_cast<PopupPosition>(ComboSelection(m_aPopupPosition));
    m_aResetPosition.EnableWindow(position == PopupPosition::LastPosition);
}

void SettingsWindow::LoadStorageControls() {
    auto &page = *m_pages[kPageStorage];
    page.LoadSettings(m_settings);
    page.ExchangeSettings(DDX_LOAD);
    m_sHistorySize.SetWindowText(std::to_wstring(m_settings.history_size).c_str());
    const std::uintmax_t storage_bytes = m_storage.StorageBytes();
    const sqlite3_int64 current_count = m_storage.CountItems();
    m_sStorageSize.SetWindowText(FormatByteCount(storage_bytes).c_str());
    const std::wstring current_size_text =
        L"（当前: " + std::to_wstring(current_count) + L" 项）";
    m_sCurrentSize.SetWindowText(current_size_text.c_str());
}

void SettingsWindow::LoadAdvancedControls() {
    auto &page = *m_pages[kPageAdvanced];
    page.LoadSettings(m_settings);
    page.ExchangeSettings(DDX_LOAD);
    m_xIgnoreNext.EnableWindow(m_settings.ignore_events);
}

void SettingsWindow::LoadControlsFromSettings() {
    m_loading = true;
    LoadGeneralControls();
    LoadAppearanceControls();
    LoadStorageControls();
    LoadAdvancedControls();

    // The whitelist belongs to the applications page.
    if (m_ignorePageObjects[0] != nullptr) {
        m_ignorePages[0]->LoadSettings(m_settings);
        m_ignorePages[0]->ExchangeSettings(DDX_LOAD);
    }

    m_loading = false;
}

void SettingsWindow::RefreshPinsList() {
    if (m_pList.m_hWnd == nullptr) {
        return;
    }

    const sqlite3_int64 previous_selection = [&] {
        const int selected = SelectedListViewItem(m_pList);
        return selected >= 0 && selected < static_cast<int>(m_pins.size())
            ? m_pins[static_cast<size_t>(selected)].id
            : 0;
    }();
    auto pins = m_storage.GetPinnedItems(PayloadMode::Metadata);
    std::stable_sort(pins.begin(), pins.end(), [](const ClipboardItem &lhs, const ClipboardItem &rhs) {
        if (lhs.first_copied_at != rhs.first_copied_at) {
            return lhs.first_copied_at < rhs.first_copied_at;
        }
        return lhs.id < rhs.id;
    });
    m_pins = std::move(pins);

    m_pList.DeleteAllItems();
    for (size_t index = 0; index < m_pins.size(); ++index) {
        const ClipboardItem &item = m_pins[index];
        LVITEMW row{};
        row.mask = LVIF_TEXT | LVIF_PARAM;
        row.iItem = static_cast<int>(index);
        row.lParam = item.id;
        row.pszText = const_cast<wchar_t *>(item.pin.c_str());
        m_pList.InsertItem(&row);

        m_pList.SetItemText(
            static_cast<int>(index),
            1,
            item.title.c_str()
        );
        std::wstring content;
        if (item.has_text) {
            content = PinTextContent(item);
            if (content.empty()) {
                content = item.preview;
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
        m_pList.SetItemText(static_cast<int>(index), 2, content.c_str());
    }

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
        m_pList.SetItemState(
            selected_index,
            LVIS_SELECTED | LVIS_FOCUSED,
            LVIS_SELECTED | LVIS_FOCUSED
        );
    }
}

void SettingsWindow::SaveCurrentPage() {
    if (m_loading) {
        return;
    }

    const AppSettings previous = m_settings;
    try {
        switch (m_currentPage) {
        case kPageGeneral:
            m_pages[kPageGeneral]->AttachSettings(m_settings);
            m_pages[kPageGeneral]->ExchangeSettings(DDX_SAVE);
            m_settings.open_hotkey = HotKeyFromControl(m_gOpenHotKey);
            m_settings.pin_hotkey = HotKeyFromControl(m_gPinHotKey);
            m_settings.delete_hotkey = HotKeyFromControl(m_gDeleteHotKey);
            m_settings.preview_hotkey = HotKeyFromControl(m_gPreviewHotKey);
            break;
        case kPageAppearance: {
            m_pages[kPageAppearance]->AttachSettings(m_settings);
            m_pages[kPageAppearance]->ExchangeSettings(DDX_SAVE);
            m_settings.image_max_height = ReadValidatedInteger(
                m_aImageHeight,
                m_settings.image_max_height,
                1,
                200
            );
            m_settings.preview_delay = ReadValidatedInteger(
                m_aPreviewDelay,
                m_settings.preview_delay,
                200,
                100000
            );
            break;
        }
        case kPageStorage:
            m_pages[kPageStorage]->AttachSettings(m_settings);
            m_pages[kPageStorage]->ExchangeSettings(DDX_SAVE);
            m_settings.history_size = ReadValidatedInteger(
                m_sHistorySize,
                m_settings.history_size,
                1,
                999
            );
            break;
        case kPageIgnore:
            if (m_ignorePageObjects[0] != nullptr) {
                m_ignorePages[0]->AttachSettings(m_settings);
                m_ignorePages[0]->ExchangeSettings(DDX_SAVE);
            }
            break;
        case kPageAdvanced:
            m_pages[kPageAdvanced]->AttachSettings(m_settings);
            m_pages[kPageAdvanced]->ExchangeSettings(DDX_SAVE);
            break;
        default:
            break;
        }

        const AppSettings snapshot = m_settings;
        m_storage.SaveSettings(snapshot);
        if (previous.history_size != snapshot.history_size) {
            m_storage.TrimUnpinned(snapshot.history_size);
        }
        if (previous.show_special_symbols != snapshot.show_special_symbols) {
            m_storage.RegenerateTitles(snapshot.show_special_symbols);
        }
        m_settings = snapshot;
        UpdateDependencies();
        NotifyOwner();
    } catch (const std::exception &error) {
        m_settings = previous;
        LoadControlsFromSettings();
        MessageBoxA(m_hWnd, error.what(), "无法保存设置", MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::NotifyOwner(std::uint32_t updateMask) {
    if ((updateMask & AppConstants::UiUpdate::kIgnoreRules) != 0 &&
        m_ignorePage >= 0 && m_ignorePage < static_cast<int>(m_ignorePageObjects.size()) &&
        m_ignorePageObjects[static_cast<size_t>(m_ignorePage)] != nullptr) {
        m_ignoredLists[static_cast<size_t>(m_ignorePage)] =
            m_ignorePageObjects[static_cast<size_t>(m_ignorePage)]->Values();
    }
    if (m_onChanged) {
        m_onChanged(m_settings, updateMask);
    }
}

void SettingsWindow::EditSelectedPin() {
    const int selected = SelectedListViewItem(m_pList);
    if (selected < 0 || selected >= static_cast<int>(m_pins.size())) {
        return;
    }

    const sqlite3_int64 item_id = m_pins[selected].id;
    try {
        const auto item = m_storage.GetItem(item_id, PayloadMode::Full);
        if (!item.has_value()) {
            return;
        }
        EditPinDialog dialog(m_settings, item->id, m_pins, *item);
        if (dialog.DoModal(m_hWnd) != IDOK) {
            return;
        }
        const sqlite3_int64 edited_id = dialog.GetItemId();
        const std::wstring key = dialog.GetKey();
        const std::wstring title = dialog.GetTitle();
        if (dialog.ContentModified()) {
            m_storage.UpdatePinnedItem(edited_id, key, title, dialog.GetContent());
        } else {
            m_storage.UpdatePinnedMetadata(edited_id, key, title);
        }
        RefreshPinsList();
        NotifyOwner();
    } catch (const std::exception &error) {
        ::MessageBoxA(m_hWnd, error.what(), "无法修改置顶项目", MB_OK | MB_ICONERROR);
    } catch (...) {
        ::MessageBoxW(m_hWnd, L"无法修改置顶项目。", L"无法修改置顶项目",
                      MB_OK | MB_ICONERROR);
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
    const sqlite3_int64 item_id = m_pins[selected].id;
    try {
        m_storage.DeleteItem(item_id);
        RefreshPinsList();
        NotifyOwner();
    } catch (const std::exception &error) {
        ::MessageBoxA(m_hWnd, error.what(), "无法删除置顶项目", MB_OK | MB_ICONERROR);
    } catch (...) {
        ::MessageBoxW(m_hWnd, L"无法删除置顶项目。", L"无法删除置顶项目",
                      MB_OK | MB_ICONERROR);
    }
}

void SettingsWindow::OpenNotificationsSettings() {
    ShellExecuteW(m_hWnd, L"open", L"ms-settings:notifications", nullptr, nullptr, SW_SHOWNORMAL);
}

void SettingsWindow::CheckForUpdatesNow() {
    if (m_updateCheckBusy) {
        ::MessageBoxW(
            m_hWnd,
            L"正在检查更新，请稍候。",
            L"检查更新",
            MB_OK | MB_ICONINFORMATION
        );
        return;
    }
    if (m_onUpdateCheck == nullptr || !m_onUpdateCheck()) {
        ::MessageBoxW(
            m_hWnd,
            L"无法开始检查更新，请稍后重试。",
            L"检查更新",
            MB_OK | MB_ICONWARNING
        );
    }
}

void SettingsWindow::ResetPopupPosition() {
    const AppSettings previous = m_settings;
    m_settings.popup_x = 0;
    m_settings.popup_y = 0;
    try {
        m_storage.SaveSettings(m_settings);
        NotifyOwner();
    } catch (const std::exception &error) {
        m_settings = previous;
        LoadControlsFromSettings();
        ::MessageBoxA(m_hWnd, error.what(), "无法保存设置", MB_OK | MB_ICONERROR);
    } catch (...) {
        m_settings = previous;
        LoadControlsFromSettings();
        ::MessageBoxW(m_hWnd, L"无法保存设置。", L"无法保存设置", MB_OK | MB_ICONERROR);
    }
}

LRESULT SettingsWindow::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    // Keep preferences as a normal top-level window so it remains visible in
    // the taskbar and Alt+Tab without inheriting the main window's topmost state.
    SetWindowText(L"偏好设置");
    m_windowIcon = LoadApplicationIcon();
    if (m_windowIcon != nullptr) {
        SetIcon(m_windowIcon, TRUE);
        SetIcon(m_windowIcon, FALSE);
    }
    ModifyStyleEx(
        WS_EX_TOOLWINDOW,
        WS_EX_APPWINDOW,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );
    SetWindowPos(
        HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED
    );

    CreateTabs();
    if (!CreatePageWindows()) {
        handled = FALSE;
        return FALSE;
    }
    BindControls();
    LoadControlsFromSettings();
    SetUpdateCheckBusy(m_updateCheckBusy);
    SetPage(kPageGeneral);
    SetIgnorePage(0);
    return TRUE;
}

LRESULT SettingsWindow::OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    SetControlFont(m_tabs);
    ConfigureIgnoreList();
    ConfigurePinsList();
    if (m_currentPage == kPageIgnore && m_ignorePageObjects[m_ignorePage] != nullptr) {
        m_ignorePageObjects[m_ignorePage]->Refresh();
    }
    return 0;
}

LRESULT SettingsWindow::OnClose(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    SaveCurrentPage();
    if (!m_destroying) {
        ShowWindow(SW_HIDE);
    } else {
        DestroyWindow();
    }
    return 0;
}

LRESULT SettingsWindow::OnCheckUpdatesCommand(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;
    if (!m_loading) {
        CheckForUpdatesNow();
    }
    return 0;
}

LRESULT SettingsWindow::OnNotificationsCommand(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;
    if (!m_loading) {
        OpenNotificationsSettings();
    }
    return 0;
}

LRESULT SettingsWindow::OnResetPositionCommand(WORD, WORD, HWND, BOOL &handled) {
    handled = TRUE;
    if (!m_loading) {
        ResetPopupPosition();
    }
    return 0;
}

LRESULT SettingsWindow::OnIgnorePageCommand(WORD, WORD id, HWND, BOOL &handled) {
    handled = TRUE;
    if (m_loading || m_ignorePageObjects[m_ignorePage] == nullptr) {
        return 0;
    }

    IgnorePageBase &page = *m_ignorePageObjects[m_ignorePage];
    bool changed = false;
    switch (id) {
    case kIAdd:
        changed = page.AddValue();
        break;
    case kIRemove:
        changed = page.RemoveValue();
        break;
    case kIReset:
        changed = page.ResetToDefaults();
        break;
    default:
        handled = FALSE;
        return 0;
    }
    if (changed) {
        NotifyOwner(AppConstants::UiUpdate::kIgnoreRules);
    }
    return 0;
}

LRESULT SettingsWindow::OnSettingChanged(WORD, WORD id, HWND, BOOL &handled) {
    handled = TRUE;
    if (m_loading) {
        return 0;
    }
    if (id == kAOpenPreview) {
        m_aPreviewDelay.EnableWindow(IsChecked(m_aOpenPreview));
    }
    if (id == kXIgnoreEvents) {
        m_xIgnoreNext.EnableWindow(IsChecked(m_xIgnoreEvents));
    }
    SaveCurrentPage();
    return 0;
}

LRESULT SettingsWindow::OnTabsSelectionChanged(int, LPNMHDR, BOOL &handled) {
    handled = TRUE;
    SaveCurrentPage();
    SetPage(m_tabs.GetCurSel());
    if (m_currentPage == kPageIgnore) {
        SetIgnorePage(m_ignorePage);
    } else if (m_currentPage == kPagePins) {
        RefreshPinsList();
    }
    return 0;
}

LRESULT SettingsWindow::OnIgnoreTabsSelectionChanged(int, LPNMHDR, BOOL &handled) {
    handled = TRUE;
    SetIgnorePage(m_ignoreTabs.GetCurSel());
    return 0;
}

LRESULT SettingsWindow::OnIgnoreListDoubleClick(int, LPNMHDR header, BOOL &handled) {
    if (m_currentPage != kPageIgnore || m_ignorePageObjects[m_ignorePage] == nullptr ||
        header == nullptr || header->hwndFrom !=
            m_ignorePageObjects[m_ignorePage]->ListWindow().m_hWnd) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    if (m_ignorePageObjects[m_ignorePage]->EditValue()) {
        NotifyOwner(AppConstants::UiUpdate::kIgnoreRules);
    }
    return 0;
}

LRESULT SettingsWindow::OnIgnoreListKeyDown(int, LPNMHDR header, BOOL &handled) {
    if (m_currentPage != kPageIgnore || m_ignorePageObjects[m_ignorePage] == nullptr ||
        header == nullptr || header->hwndFrom !=
            m_ignorePageObjects[m_ignorePage]->ListWindow().m_hWnd) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    const auto *key = reinterpret_cast<const NMLVKEYDOWN *>(header);
    if (key->wVKey == VK_DELETE && m_ignorePageObjects[m_ignorePage]->RemoveValue()) {
        NotifyOwner(AppConstants::UiUpdate::kIgnoreRules);
    }
    return 0;
}

LRESULT SettingsWindow::OnPinsListDoubleClick(int, LPNMHDR header, BOOL &handled) {
    handled = header != nullptr && header->hwndFrom == m_pList.m_hWnd;
    if (handled) {
        EditSelectedPin();
    }
    return 0;
}

LRESULT SettingsWindow::OnPinsListKeyDown(int, LPNMHDR header, BOOL &handled) {
    handled = header != nullptr && header->hwndFrom == m_pList.m_hWnd;
    if (!handled) {
        return 0;
    }
    const auto *key = reinterpret_cast<const NMLVKEYDOWN *>(header);
    if (key->wVKey == VK_DELETE) {
        DeleteSelectedPin();
    }
    return 0;
}

LRESULT SettingsWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    if (m_windowIcon != nullptr) {
        SetIcon(nullptr, TRUE);
        SetIcon(nullptr, FALSE);
        ::DestroyIcon(m_windowIcon);
        m_windowIcon = nullptr;
    }
    if (!m_ignoreImageList.IsNull()) {
        m_ignoreImageList.Destroy();
    }
    m_hWnd = nullptr;
    return 0;
}

namespace {

void SetIgnoreListViewColumnWidth(CListViewCtrl list, int width) {
    if (list.m_hWnd != nullptr) {
        list.SetColumnWidth(0, width);
    }
}

}

void IgnorePageBase::SetValues(std::vector<std::wstring> values) {
    m_values = std::move(values);
    m_persistedValues = m_values;
    Refresh();
}

bool IgnorePageBase::PersistValues(IgnoreListKind list) {
    if (m_storage == nullptr) {
        return false;
    }
    try {
        m_storage->ReplaceList(list, m_values);
        m_persistedValues = m_values;
        return true;
    } catch (const std::exception &error) {
        m_values = m_persistedValues;
        Refresh();
        ::MessageBoxA(m_pageWindow.m_hWnd, error.what(), "无法保存忽略规则", MB_OK | MB_ICONERROR);
        return false;
    } catch (...) {
        m_values = m_persistedValues;
        Refresh();
        ::MessageBoxW(m_pageWindow.m_hWnd, L"无法保存忽略规则。", L"无法保存忽略规则", MB_OK | MB_ICONERROR);
        return false;
    }
}

void IgnoreApplicationsPage::Initialize(
    CWindow page_window,
    StorageWorker &storage,
    std::vector<std::wstring> values
) {
    m_pageWindow = page_window;
    m_storage = &storage;
    SetValues(std::move(values));
    m_list = page_window.GetDlgItem(IDC_I_LIST);
    m_description = page_window.GetDlgItem(IDC_I_DESCRIPTION);

    if (m_list.m_hWnd != nullptr) {
        LVCOLUMNW column{};
        column.mask = LVCF_WIDTH;
        column.cx = 400;
        m_list.InsertColumn(0, &column);
    }

    UpdateDescription();
}

void IgnoreApplicationsPage::Show() {
    if (m_pageWindow.m_hWnd != nullptr) {
        m_pageWindow.ShowWindow(SW_SHOW);
        Refresh();
    }
}

void IgnoreApplicationsPage::Hide() {
    if (m_pageWindow.m_hWnd != nullptr) {
        m_pageWindow.ShowWindow(SW_HIDE);
    }
}

void IgnoreApplicationsPage::Refresh() {
    if (m_list.m_hWnd == nullptr) {
        return;
    }

    m_list.DeleteAllItems();
    for (size_t index = 0; index < m_values.size(); ++index) {
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = static_cast<int>(index);
        row.pszText = const_cast<wchar_t *>(m_values[index].c_str());
        m_list.InsertItem(&row);
    }

    RECT list_rect{};
    m_list.GetClientRect(&list_rect);
    SetIgnoreListViewColumnWidth(m_list, list_rect.right - list_rect.left);
}

bool IgnoreApplicationsPage::AddValue() {
    std::array<wchar_t, MAX_PATH> path{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = m_pageWindow.m_hWnd;
    dialog.lpstrFilter = L"Windows application (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog)) {
        return false;
    }

    std::wstring value = path.data();
    if (value.empty() || std::find(m_values.begin(), m_values.end(), value) != m_values.end()) {
        return false;
    }

    m_values.push_back(std::move(value));
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreApplicationsPage::EditValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    std::array<wchar_t, MAX_PATH> path{};
    wcscpy_s(path.data(), path.size(), m_values[selected].c_str());

    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = m_pageWindow.m_hWnd;
    dialog.lpstrFilter = L"Windows application (*.exe)\0*.exe\0All files (*.*)\0*.*\0\0";
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (!GetOpenFileNameW(&dialog)) {
        return false;
    }

    std::wstring value = path.data();
    if (value.empty()) {
        return false;
    }

    for (size_t index = 0; index < m_values.size(); ++index) {
        if (static_cast<int>(index) != selected && m_values[index] == value) {
            MessageBoxW(m_pageWindow.m_hWnd, L"该应用程序已存在。", L"错误", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    m_values[selected] = value;
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreApplicationsPage::RemoveValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    m_values.erase(m_values.begin() + selected);
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreApplicationsPage::ResetToDefaults() {
    return false;
}

bool IgnoreApplicationsPage::SaveList() {
    return PersistValues(IgnoreListKind::Applications);
}

void IgnoreApplicationsPage::UpdateDescription() {
    if (m_description.m_hWnd != nullptr) {
        m_description.SetWindowText(
            L"忽略来自特定应用的内容。\r\n请注意此选项并非总是有效，最好使用忽略剪贴板类型设置。"
        );
    }
}

void IgnoreFormatsPage::Initialize(
    CWindow page_window,
    StorageWorker &storage,
    std::vector<std::wstring> values
) {
    m_pageWindow = page_window;
    m_storage = &storage;
    SetValues(std::move(values));
    m_list = page_window.GetDlgItem(IDC_I_LIST);
    m_description = page_window.GetDlgItem(IDC_I_DESCRIPTION);

    if (m_list.m_hWnd != nullptr) {
        LVCOLUMNW column{};
        column.mask = LVCF_WIDTH;
        column.cx = 400;
        m_list.InsertColumn(0, &column);
    }

    UpdateDescription();
}

void IgnoreFormatsPage::Show() {
    if (m_pageWindow.m_hWnd != nullptr) {
        m_pageWindow.ShowWindow(SW_SHOW);
        Refresh();
    }
}

void IgnoreFormatsPage::Hide() {
    if (m_pageWindow.m_hWnd != nullptr) {
        m_pageWindow.ShowWindow(SW_HIDE);
    }
}

void IgnoreFormatsPage::Refresh() {
    if (m_list.m_hWnd == nullptr) {
        return;
    }

    m_list.DeleteAllItems();
    for (size_t index = 0; index < m_values.size(); ++index) {
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = static_cast<int>(index);
        row.pszText = const_cast<wchar_t *>(m_values[index].c_str());
        m_list.InsertItem(&row);
    }

    RECT list_rect{};
    m_list.GetClientRect(&list_rect);
    SetIgnoreListViewColumnWidth(m_list, list_rect.right - list_rect.left);
}

bool IgnoreFormatsPage::AddValue() {
    EditIgnoreDialog dialog(
        L"",
        L"输入要忽略的 pasteboard 类型（例如：com.example.custom）。",
        1
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty() || std::find(m_values.begin(), m_values.end(), value) != m_values.end()) {
        return false;
    }

    m_values.push_back(std::move(value));
    const bool saved = SaveList();
    Refresh();

    const int inserted = static_cast<int>(m_values.size() - 1);
    if (m_list.m_hWnd != nullptr && inserted >= 0) {
        m_list.SetItemState(inserted, LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);
    }
    return saved;
}

bool IgnoreFormatsPage::EditValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    EditIgnoreDialog dialog(
        m_values[selected],
        L"编辑要忽略的 pasteboard 类型（例如：com.example.custom）。",
        1
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty()) {
        return false;
    }

    for (size_t index = 0; index < m_values.size(); ++index) {
        if (static_cast<int>(index) != selected && m_values[index] == value) {
            MessageBoxW(m_pageWindow.m_hWnd, L"该值已存在。", L"错误", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    m_values[selected] = value;
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreFormatsPage::RemoveValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    m_values.erase(m_values.begin() + selected);
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreFormatsPage::ResetToDefaults() {
    m_values = ClipboardRules::DefaultIgnoredFormats();
    if (!SaveList()) {
        return false;
    }
    Refresh();
    return true;
}

bool IgnoreFormatsPage::SaveList() {
    return PersistValues(IgnoreListKind::Formats);
}

void IgnoreFormatsPage::UpdateDescription() {
    if (m_description.m_hWnd != nullptr) {
        m_description.SetWindowText(
            L"忽略特定剪贴板内容类型。\r\n默认提供了一些已知的适用于特定应用的类型。您可以删除预置类型，或根据需要添加自定义类型。"
        );
    }
}

void IgnoreRegexpsPage::Initialize(
    CWindow page_window,
    StorageWorker &storage,
    std::vector<std::wstring> values
) {
    m_pageWindow = page_window;
    m_storage = &storage;
    SetValues(std::move(values));
    m_list = page_window.GetDlgItem(IDC_I_LIST);
    m_description = page_window.GetDlgItem(IDC_I_DESCRIPTION);

    if (m_list.m_hWnd != nullptr) {
        LVCOLUMNW column{};
        column.mask = LVCF_WIDTH;
        column.cx = 400;
        m_list.InsertColumn(0, &column);
    }

    UpdateDescription();
}

void IgnoreRegexpsPage::Show() {
    if (m_pageWindow.m_hWnd != nullptr) {
        m_pageWindow.ShowWindow(SW_SHOW);
        Refresh();
    }
}

void IgnoreRegexpsPage::Hide() {
    if (m_pageWindow.m_hWnd != nullptr) {
        m_pageWindow.ShowWindow(SW_HIDE);
    }
}

void IgnoreRegexpsPage::Refresh() {
    if (m_list.m_hWnd == nullptr) {
        return;
    }

    m_list.DeleteAllItems();
    for (size_t index = 0; index < m_values.size(); ++index) {
        LVITEMW row{};
        row.mask = LVIF_TEXT;
        row.iItem = static_cast<int>(index);
        row.pszText = const_cast<wchar_t *>(m_values[index].c_str());
        m_list.InsertItem(&row);
    }

    RECT list_rect{};
    m_list.GetClientRect(&list_rect);
    SetIgnoreListViewColumnWidth(m_list, list_rect.right - list_rect.left);
}

bool IgnoreRegexpsPage::AddValue() {
    EditIgnoreDialog dialog(
        L"",
        L"输入正则表达式以忽略匹配的内容（例如：^[a-zA-Z0-9]{50}$）。",
        2
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty() || std::find(m_values.begin(), m_values.end(), value) != m_values.end()) {
        return false;
    }

    m_values.push_back(std::move(value));
    const bool saved = SaveList();
    Refresh();

    const int inserted = static_cast<int>(m_values.size() - 1);
    if (m_list.m_hWnd != nullptr && inserted >= 0) {
        m_list.SetItemState(inserted, LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);
    }
    return saved;
}

bool IgnoreRegexpsPage::EditValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    EditIgnoreDialog dialog(
        m_values[selected],
        L"编辑正则表达式以忽略匹配的内容（例如：^[a-zA-Z0-9]{50}$）。",
        2
    );

    if (dialog.DoModal(m_pageWindow) != IDOK) {
        return false;
    }

    std::wstring value = dialog.GetValue();
    if (value.empty()) {
        return false;
    }

    for (size_t index = 0; index < m_values.size(); ++index) {
        if (static_cast<int>(index) != selected && m_values[index] == value) {
            MessageBoxW(m_pageWindow.m_hWnd, L"该值已存在。", L"错误", MB_OK | MB_ICONWARNING);
            return false;
        }
    }

    m_values[selected] = value;
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreRegexpsPage::RemoveValue() {
    const int selected = SelectedListViewItem(m_list);
    if (selected < 0 || selected >= static_cast<int>(m_values.size())) {
        return false;
    }

    m_values.erase(m_values.begin() + selected);
    const bool saved = SaveList();
    Refresh();
    return saved;
}

bool IgnoreRegexpsPage::ResetToDefaults() {
    return false;
}

bool IgnoreRegexpsPage::SaveList() {
    return PersistValues(IgnoreListKind::Regexps);
}

void IgnoreRegexpsPage::UpdateDescription() {
    if (m_description.m_hWnd != nullptr) {
        m_description.SetWindowText(L"可以根据定义的正则表达式忽略某些副本。");
    }
}
