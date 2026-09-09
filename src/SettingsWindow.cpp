#define NOMINMAX
#include "SettingsWindow.h"

#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <filesystem>
#include <string>

namespace {

enum SettingsControlId : int {
    kTabs = 7000,

    kGLaunch = 7100,
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

    kAPopupPosition = 7200,
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

    kSSaveFiles = 7300,
    kSSaveImages,
    kSSaveText,
    kSHistorySize,
    kSSortBy,

    kIgnoreTabs = 7400,
    kIList,
    kIEdit,
    kIAdd,
    kIBrowse,
    kIUpdate,
    kIRemove,
    kIReset,
    kIWhitelist,

    kPList = 7500,
    kPKey,
    kPTitle,
    kPContent,
    kPSave,
    kPDelete,

    kXIgnoreEvents = 7600,
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

SettingsWindow::SettingsWindow(Database &database, HWND owner)
    : m_database(database), m_owner(owner) {}

bool SettingsWindow::CreateOrShow() {
    if (m_hWnd != nullptr && ::IsWindow(m_hWnd)) {
        ShowWindow(SW_SHOW);
        SetForegroundWindow(m_hWnd);
        return true;
    }

    m_destroying = false;
    const HWND window = Create(
        m_owner,
        CWindow::rcDefault,
        L"剪贴板设置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
        WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT
    );
    if (window == nullptr) {
        return false;
    }

    constexpr int kWindowWidth = 780;
    constexpr int kWindowHeight = 650;
    HMONITOR monitor = ::MonitorFromWindow(m_owner, MONITOR_DEFAULTTONEAREST);
    if (monitor == nullptr) {
        monitor = ::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    }

    RECT work_area{0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN)};
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor != nullptr && ::GetMonitorInfoW(monitor, &monitor_info)) {
        work_area = monitor_info.rcWork;
    }

    const int x = work_area.left + ((work_area.right - work_area.left) - kWindowWidth) / 2;
    const int y = work_area.top + ((work_area.bottom - work_area.top) - kWindowHeight) / 2;
    ::SetWindowPos(window, HWND_TOP, x, y, kWindowWidth, kWindowHeight, SWP_SHOWWINDOW);
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
    bool stretch_height
) {
    if (window == nullptr || page < 0 || page >= kPageCount) {
        return;
    }
    SetControlFont(window);
    m_pageControls[static_cast<size_t>(page)].push_back(
        LayoutControl{window, relative, stretch_width, stretch_height}
    );
}

HWND SettingsWindow::AddStatic(int page, const wchar_t *text, RECT relative, DWORD style) {
    CStatic control;
    const HWND window = control.Create(
        m_hWnd,
        CWindow::rcDefault,
        text,
        WS_CHILD | style,
        0U,
        0U
    );
    AddLayout(page, window, relative);
    return window;
}

HWND SettingsWindow::AddButton(int page, const wchar_t *text, int id, RECT relative, DWORD style) {
    CButton control;
    const HWND window = control.Create(
        m_hWnd,
        CWindow::rcDefault,
        text,
        WS_CHILD | WS_TABSTOP | style,
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
        m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_TABSTOP | WS_BORDER | style,
        WS_EX_CLIENTEDGE,
        id
    );
    AddLayout(page, window, relative);
    return window;
}

HWND SettingsWindow::AddCombo(int page, int id, RECT relative) {
    CComboBox control;
    const HWND window = control.Create(
        m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
        0,
        id
    );
    AddLayout(page, window, relative);
    return window;
}

HWND SettingsWindow::AddList(int page, int id, RECT relative, bool stretch_height) {
    CListBox control;
    const HWND window = control.Create(
        m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_TABSTOP | WS_VSCROLL | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
        WS_EX_CLIENTEDGE,
        id
    );
    // Lists use the available page width but keep a predictable height so
    // the lightweight editor controls below them remain visible.
    relative.right = 8;
    AddLayout(page, window, relative, true, false);
    (void)stretch_height;
    return window;
}

HWND SettingsWindow::AddHotKey(int page, int id, RECT relative) {
    CHotKeyCtrl control;
    const HWND window = control.Create(
        m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_TABSTOP,
        WS_EX_CLIENTEDGE,
        id
    );
    AddLayout(page, window, relative);
    return window;
}

void SettingsWindow::CreateTabs() {
    m_tabWindow = m_tabs.Create(
        m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | TCS_TABS,
        0,
        kTabs
    );
    SetControlFont(m_tabWindow);

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

void SettingsWindow::CreateGeneralPage() {
    m_gLaunch = AddCheckBox(kPageGeneral, L"登录 Windows 时启动", kGLaunch, {16, 12, 260, 38});
    m_gUpdates = AddCheckBox(kPageGeneral, L"自动检查更新（仅保存选项）", kGUpdates, {16, 46, 310, 72});
    AddButton(kPageGeneral, L"立即检查", kGCheckNow, {330, 42, 430, 72});
    AddStatic(
        kPageGeneral,
        L"Windows 版本没有 Sparkle 更新服务；“立即检查”会打开项目发布页。",
        {16, 78, 740, 112},
        SS_LEFT | SS_NOPREFIX
    );
    AddStatic(kPageGeneral, L"打开：", {16, 126, 110, 152});
    m_gOpenHotKey = AddHotKey(kPageGeneral, kGOpenHotKey, {120, 126, 285, 154});
    AddStatic(kPageGeneral, L"置顶：", {16, 164, 110, 190});
    m_gPinHotKey = AddHotKey(kPageGeneral, kGPinHotKey, {120, 164, 285, 192});
    AddStatic(kPageGeneral, L"删除：", {16, 202, 110, 228});
    m_gDeleteHotKey = AddHotKey(kPageGeneral, kGDeleteHotKey, {120, 202, 285, 230});
    AddStatic(kPageGeneral, L"预览：", {16, 240, 110, 266});
    m_gPreviewHotKey = AddHotKey(kPageGeneral, kGPreviewHotKey, {120, 240, 285, 268});

    AddStatic(kPageGeneral, L"搜索模式：", {16, 286, 140, 314});
    m_gSearchMode = AddCombo(kPageGeneral, kGSearchMode, {145, 282, 320, 312});
    AddComboItem(m_gSearchMode, L"精确（不区分大小写）");
    AddComboItem(m_gSearchMode, L"模糊");
    AddComboItem(m_gSearchMode, L"正则表达式");
    AddComboItem(m_gSearchMode, L"混合（精确→正则→模糊）");

    m_gPasteByDefault = AddCheckBox(kPageGeneral, L"选择项目后自动粘贴", kGPasteByDefault, {16, 334, 300, 360});
    m_gRemoveFormatting = AddCheckBox(kPageGeneral, L"默认粘贴为纯文本（去除格式）", kGRemoveFormatting, {16, 368, 350, 394});
    AddButton(kPageGeneral, L"Windows 通知和声音设置", kGNotifications, {16, 420, 250, 450});
}

void SettingsWindow::CreateAppearancePage() {
    AddStatic(kPageAppearance, L"弹出位置：", {16, 12, 140, 40});
    m_aPopupPosition = AddCombo(kPageAppearance, kAPopupPosition, {145, 8, 345, 38});
    AddComboItem(m_aPopupPosition, L"光标附近");
    AddComboItem(m_aPopupPosition, L"托盘图标附近");
    AddComboItem(m_aPopupPosition, L"目标窗口中心");
    AddComboItem(m_aPopupPosition, L"屏幕中心");
    AddComboItem(m_aPopupPosition, L"上次位置");
    m_aPopupScreen = AddCombo(kPageAppearance, kAPopupScreen, {500, 8, 650, 38});
    AddButton(kPageAppearance, L"重置位置", kAResetPosition, {660, 8, 750, 38});

    AddStatic(kPageAppearance, L"置顶项目位置：", {16, 52, 160, 80});
    m_aPinTo = AddCombo(kPageAppearance, kAPinTo, {180, 48, 340, 78});
    AddComboItem(m_aPinTo, L"顶部");
    AddComboItem(m_aPinTo, L"底部");

    AddStatic(kPageAppearance, L"图片最大高度：", {16, 92, 160, 120});
    m_aImageHeight = AddEdit(kPageAppearance, kAImageHeight, {180, 88, 260, 116}, ES_NUMBER);
    AddStatic(kPageAppearance, L"像素（1–200）", {270, 92, 390, 120});
    m_aOpenPreview = AddCheckBox(kPageAppearance, L"自动打开预览", kAOpenPreview, {16, 132, 260, 158});
    AddStatic(kPageAppearance, L"预览延迟：", {16, 170, 160, 198});
    m_aPreviewDelay = AddEdit(kPageAppearance, kAPreviewDelay, {180, 166, 260, 194}, ES_NUMBER);
    AddStatic(kPageAppearance, L"毫秒（200–100000）", {270, 170, 430, 198});

    AddStatic(kPageAppearance, L"搜索匹配样式：", {16, 210, 170, 238});
    m_aHighlight = AddCombo(kPageAppearance, kAHighlight, {180, 206, 340, 236});
    AddComboItem(m_aHighlight, L"颜色");
    AddComboItem(m_aHighlight, L"粗体");
    AddComboItem(m_aHighlight, L"斜体");
    AddComboItem(m_aHighlight, L"下划线");

    AddStatic(kPageAppearance, L"托盘图标：", {16, 250, 140, 278});
    m_aMenuIcon = AddCombo(kPageAppearance, kAMenuIcon, {145, 246, 330, 276});
    AddComboItem(m_aMenuIcon, L"Maccy");
    AddComboItem(m_aMenuIcon, L"剪贴板");
    AddComboItem(m_aMenuIcon, L"剪刀");
    AddComboItem(m_aMenuIcon, L"回形针");
    m_aShowStatus = AddCheckBox(kPageAppearance, L"显示托盘图标", kAShowStatus, {350, 246, 500, 278});
    m_aShowRecent = AddCheckBox(kPageAppearance, L"在托盘提示中显示最近复制内容", kAShowRecent, {16, 286, 360, 312});
    m_aShowSearch = AddCheckBox(kPageAppearance, L"显示搜索框", kAShowSearch, {16, 320, 180, 346});
    m_aSearchVisibility = AddCombo(kPageAppearance, kASearchVisibility, {200, 316, 360, 346});
    AddComboItem(m_aSearchVisibility, L"始终显示");
    AddComboItem(m_aSearchVisibility, L"搜索时显示");
    m_aShowTitle = AddCheckBox(kPageAppearance, L"在搜索框前显示标题", kAShowTitle, {16, 354, 260, 380});
    m_aShowIcons = AddCheckBox(kPageAppearance, L"显示来源程序图标", kAShowIcons, {16, 388, 260, 414});
    m_aShowSwatch = AddCheckBox(kPageAppearance, L"显示十六进制颜色色块", kAShowSwatch, {280, 388, 520, 414});
    m_aShowSpecial = AddCheckBox(kPageAppearance, L"显示换行、制表符和首尾空格符号", kAShowSpecial, {16, 422, 350, 448});
    m_aShowFooter = AddCheckBox(kPageAppearance, L"显示底部状态栏", kAShowFooter, {16, 456, 250, 482});
}

void SettingsWindow::CreateStoragePage() {
    AddStatic(kPageStorage, L"保存：", {16, 12, 120, 40});
    m_sSaveFiles = AddCheckBox(kPageStorage, L"文件（CF_HDROP）", kSSaveFiles, {40, 48, 260, 76});
    m_sSaveImages = AddCheckBox(kPageStorage, L"图片（DIB/DIBV5）", kSSaveImages, {40, 82, 260, 110});
    m_sSaveText = AddCheckBox(kPageStorage, L"文本（Unicode/HTML/RTF）", kSSaveText, {40, 116, 300, 144});
    AddStatic(
        kPageStorage,
        L"关闭某种类型后，新的剪贴板内容不会保存该类型；已有历史不会被删除。",
        {16, 154, 740, 198},
        SS_LEFT | SS_NOPREFIX
    );
    AddStatic(kPageStorage, L"保留历史数量：", {16, 218, 160, 246});
    m_sHistorySize = AddEdit(kPageStorage, kSHistorySize, {180, 214, 260, 242}, ES_NUMBER);
    AddStatic(kPageStorage, L"条（1–999，不含置顶项）", {270, 218, 500, 246});
    AddStatic(kPageStorage, L"排序：", {16, 258, 120, 286});
    m_sSortBy = AddCombo(kPageStorage, kSSortBy, {180, 254, 390, 284});
    AddComboItem(m_sSortBy, L"最近复制时间");
    AddComboItem(m_sSortBy, L"首次复制时间");
    AddComboItem(m_sSortBy, L"复制次数");
    AddStatic(kPageStorage, L"当前数据库大小：", {16, 300, 180, 328});
    m_sStorageSize = AddStatic(kPageStorage, L"", {190, 300, 350, 328});
}

void SettingsWindow::CreateIgnorePage() {
    m_ignoreTabWindow = m_ignoreTabs.Create(
        m_hWnd,
        CWindow::rcDefault,
        nullptr,
        WS_CHILD | TCS_TABS,
        0,
        kIgnoreTabs
    );
    AddLayout(kPageIgnore, m_ignoreTabWindow, {8, 8, 650, 42}, true, false);
    const std::array<const wchar_t *, 3> names = {L"应用程序", L"剪贴板格式", L"正则表达式"};
    for (const wchar_t *name : names) {
        TCITEMW item{};
        item.mask = TCIF_TEXT;
        item.pszText = const_cast<wchar_t *>(name);
        m_ignoreTabs.InsertItem(m_ignoreTabs.GetItemCount(), &item);
    }
    m_iList = AddList(kPageIgnore, kIList, {8, 52, 650, 330}, true);
    m_iEdit = AddEdit(kPageIgnore, kIEdit, {8, 348, 430, 376});
    AddButton(kPageIgnore, L"添加", kIAdd, {440, 346, 510, 378});
    AddButton(kPageIgnore, L"浏览…", kIBrowse, {516, 346, 586, 378});
    AddButton(kPageIgnore, L"修改", kIUpdate, {592, 346, 662, 378});
    AddButton(kPageIgnore, L"删除", kIRemove, {668, 346, 738, 378});
    AddButton(kPageIgnore, L"恢复默认", kIReset, {592, 386, 738, 418});
    m_iWhitelist = AddCheckBox(kPageIgnore, L"仅忽略列表中的应用（白名单）", kIWhitelist, {8, 386, 330, 418});
    m_iDescription = AddStatic(kPageIgnore, L"", {8, 432, 740, 500}, SS_LEFT | SS_NOPREFIX);
}

void SettingsWindow::CreatePinsPage() {
    m_pList = AddList(kPagePins, kPList, {8, 8, 650, 250}, true);
    AddStatic(kPagePins, L"按键：", {8, 274, 90, 300});
    m_pKey = AddEdit(kPagePins, kPKey, {100, 270, 230, 300});
    AddStatic(kPagePins, L"标题：", {8, 310, 90, 336});
    m_pTitle = AddEdit(kPagePins, kPTitle, {100, 306, 650, 336}, ES_AUTOHSCROLL);
    AddStatic(kPagePins, L"内容：", {8, 346, 90, 372});
    m_pContent = AddEdit(kPagePins, kPContent, {100, 342, 650, 440}, ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL);
    m_pContentHint = AddStatic(
        kPagePins,
        L"只能编辑纯文本；图片和文件可以修改按键与标题，但内容保持原样。",
        {8, 450, 650, 480},
        SS_LEFT | SS_NOPREFIX
    );
    AddButton(kPagePins, L"保存置顶项", kPSave, {520, 490, 650, 522});
    AddButton(kPagePins, L"删除置顶项", kPDelete, {660, 490, 760, 522});
}

void SettingsWindow::CreateAdvancedPage() {
    m_xIgnoreEvents = AddCheckBox(kPageAdvanced, L"暂时忽略所有新的复制", kXIgnoreEvents, {16, 14, 300, 42});
    AddStatic(
        kPageAdvanced,
        L"开启后不会记录新的剪贴板变化；“只忽略下一次”会在下一次变化后自动关闭。",
        {16, 50, 740, 94},
        SS_LEFT | SS_NOPREFIX
    );
    m_xIgnoreNext = AddCheckBox(kPageAdvanced, L"只忽略下一次复制", kXIgnoreNext, {16, 108, 280, 136});
    AddStatic(
        kPageAdvanced,
        L"macOS 中可通过 Option 点击菜单图标临时切换；Windows 版提供此处的等价设置。",
        {16, 144, 740, 188},
        SS_LEFT | SS_NOPREFIX
    );
    m_xClearOnQuit = AddCheckBox(kPageAdvanced, L"退出时清空历史", kXClearOnQuit, {16, 222, 280, 250});
    AddStatic(kPageAdvanced, L"只删除未置顶项目。", {300, 222, 500, 250});
    m_xClearClipboard = AddCheckBox(kPageAdvanced, L"同时清空系统剪贴板", kXClearClipboard, {16, 264, 300, 292});
    AddStatic(kPageAdvanced, L"启用后，清空历史也会调用 EmptyClipboard。", {300, 264, 600, 292});
}

RECT SettingsWindow::PageRect() const {
    RECT page{};
    if (m_tabWindow == nullptr) {
        return page;
    }
    ::GetClientRect(m_tabWindow, &page);
    TabCtrl_AdjustRect(m_tabWindow, FALSE, &page);
    return page;
}

void SettingsWindow::LayoutControls() {
    if (m_tabWindow == nullptr) {
        return;
    }

    RECT client{};
    ::GetClientRect(m_hWnd, &client);
    const int width = std::max<int>(0, static_cast<int>(client.right - client.left));
    const int height = std::max<int>(0, static_cast<int>(client.bottom - client.top));
    m_tabs.MoveWindow(8, 8, std::max<int>(0, width - 16), std::max<int>(0, height - 16), TRUE);
    const RECT page = PageRect();

    for (int page_index = 0; page_index < kPageCount; ++page_index) {
        for (const LayoutControl &layout : m_pageControls[static_cast<size_t>(page_index)]) {
            RECT target{};
            target.left = page.left + layout.relative.left;
            target.top = page.top + layout.relative.top;
            target.right = layout.stretch_width
                ? page.right - layout.relative.right
                : page.left + layout.relative.right;
            target.bottom = layout.stretch_height
                ? page.bottom - layout.relative.bottom
                : page.top + layout.relative.bottom;
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
}

void SettingsWindow::SetPage(int page) {
    m_currentPage = std::clamp(page, 0, kPageCount - 1);
    m_tabs.SetCurSel(m_currentPage);
    for (int page_index = 0; page_index < kPageCount; ++page_index) {
        const bool visible = page_index == m_currentPage;
        for (const LayoutControl &layout : m_pageControls[static_cast<size_t>(page_index)]) {
            ::ShowWindow(layout.window, visible ? SW_SHOW : SW_HIDE);
        }
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
    ::ShowWindow(::GetDlgItem(m_hWnd, kIBrowse), applications ? SW_SHOW : SW_HIDE);
    ::ShowWindow(::GetDlgItem(m_hWnd, kIReset), formats ? SW_SHOW : SW_HIDE);
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
            try {
                m_settings.image_max_height = std::clamp(std::stoi(ReadWindowText(m_aImageHeight)), 1, 200);
            } catch (...) {
            }
            m_settings.open_preview_automatically = IsChecked(m_aOpenPreview);
            try {
                m_settings.preview_delay = std::clamp(std::stoi(ReadWindowText(m_aPreviewDelay)), 200, 100000);
            } catch (...) {
            }
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
            try {
                m_settings.history_size = std::clamp(std::stoi(ReadWindowText(m_sHistorySize)), 1, 999);
            } catch (...) {
            }
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

LRESULT SettingsWindow::OnCreate(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    m_settings = AppSettings::Load(m_database);
    CreateTabs();
    CreateGeneralPage();
    CreateAppearancePage();
    CreateStoragePage();
    CreateIgnorePage();
    CreatePinsPage();
    CreateAdvancedPage();
    LoadControlsFromSettings();
    SetPage(kPageGeneral);
    SetIgnorePage(0);
    return 0;
}

LRESULT SettingsWindow::OnSize(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
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
    m_tabWindow = nullptr;
    if (!m_destroying) {
        m_hWnd = nullptr;
    }
    return 0;
}
