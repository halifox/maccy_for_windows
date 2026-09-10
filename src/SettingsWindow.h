#pragma once

#define NOMINMAX
#include <windows.h>

#include <array>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlframe.h>
#include <atlwin.h>

#include "Database.h"
#include "Settings.h"
#include "resource.h"

constexpr UINT kSettingsChangedMessage = WM_APP + 20;

// A page is a real child window of the tab control.  Keeping its layout and
// scroll state here prevents the settings window from mixing tab-local and
// parent-window coordinates.
class SettingsPageWindow : public CWindowImpl<SettingsPageWindow> {
public:
    DECLARE_WND_CLASS_EX(L"ClipboardSettingsPage", CS_HREDRAW | CS_VREDRAW, COLOR_BTNFACE)

    struct LayoutControl {
        HWND window = nullptr;
        RECT design{};
        bool stretch_width = false;
        bool stretch_height = false;
        bool combo = false;
    };

    void Configure(HWND owner, bool scrollable, int design_width, int design_height);
    void AddLayout(HWND window, RECT design, bool stretch_width, bool stretch_height, bool combo);
    void SetContentSize(int design_width, int design_height);
    void LayoutControls();
    int Scale(int value) const noexcept;

    BEGIN_MSG_MAP(SettingsPageWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    END_MSG_MAP()

private:
    void UpdateDpi();
    void SetScrollPosition(int position);
    void ScrollBy(int delta);
    int MaxScrollPosition(const RECT &client) const;

    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnVScroll(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnMouseWheel(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);

    HWND m_owner = nullptr;
    std::vector<LayoutControl> m_controls;
    int m_designWidth = 760;
    int m_designHeight = 520;
    bool m_scrollable = false;
    int m_scrollY = 0;
    UINT m_dpi = USER_DEFAULT_SCREEN_DPI;
};

class SettingsWindow : public CDialogImpl<SettingsWindow>, public CDialogResize<SettingsWindow> {
public:
    enum { IDD = IDD_SETTINGS };

    SettingsWindow(Database &database, HWND owner);

    bool CreateOrShow();
    void DestroyForOwner();
    bool IsOpen() const noexcept { return m_hWnd != nullptr && IsWindowVisible(); }
    HWND Window() const noexcept { return m_hWnd; }

    BEGIN_DLGRESIZE_MAP(SettingsWindow)
        DLGRESIZE_CONTROL(IDC_SETTINGS_TABS, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_MSG_MAP(SettingsWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        CHAIN_MSG_MAP(CDialogResize<SettingsWindow>)
    END_MSG_MAP()

private:
    static constexpr int kPageCount = 6;

    HWND AddStatic(int page, const wchar_t *text, RECT relative, DWORD style = SS_LEFT);
    HWND AddSectionHeading(int page, const wchar_t *text, RECT relative);
    HWND AddButton(int page, const wchar_t *text, int id, RECT relative, DWORD style = BS_PUSHBUTTON);
    HWND AddCheckBox(int page, const wchar_t *text, int id, RECT relative);
    HWND AddEdit(int page, int id, RECT relative, DWORD style = ES_AUTOHSCROLL);
    HWND AddCombo(int page, int id, RECT relative);
    HWND AddList(int page, int id, RECT relative, bool stretch_height = true);
    HWND AddHotKey(int page, int id, RECT relative);
    void AddLayout(
        int page,
        HWND window,
        RECT relative,
        bool stretch_width = false,
        bool stretch_height = false,
        bool combo = false
    );

    void CreateTabs();
    bool CreatePageWindows();
    void CreateGeneralPage();
    void CreateAppearancePage();
    void CreateStoragePage();
    void CreateIgnorePage();
    void CreatePinsPage();
    void CreateAdvancedPage();

    void SetPage(int page);
    void SetIgnorePage(int page);
    void LayoutControls();
    RECT PageRect() const;

    void LoadControlsFromSettings();
    void LoadGeneralControls();
    void LoadAppearanceControls();
    void LoadStorageControls();
    void LoadAdvancedControls();
    void RefreshIgnoreList();
    void RefreshPinsList();
    void LoadSelectedPin();

    void SaveCurrentPage(bool notify = true);
    void NotifyOwner();
    void SaveIgnoreList();
    void AddIgnoreValue(bool browse_for_application);
    void UpdateIgnoreValue();
    void RemoveIgnoreValue();
    void ResetIgnoredFormats();
    void SaveSelectedPin();
    void DeleteSelectedPin();
    void OpenNotificationsSettings();
    void CheckForUpdatesNow();
    void ResetPopupPosition();

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);

    Database &m_database;
    HWND m_owner = nullptr;
    CTabCtrl m_tabs;
    std::array<SettingsPageWindow, kPageCount> m_pages;
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_destroying = false;
    HFONT m_sectionFont = nullptr;

    AppSettings m_settings{};

    // General page.
    HWND m_gLaunch = nullptr;
    HWND m_gUpdates = nullptr;
    HWND m_gOpenHotKey = nullptr;
    HWND m_gPinHotKey = nullptr;
    HWND m_gDeleteHotKey = nullptr;
    HWND m_gPreviewHotKey = nullptr;
    HWND m_gSearchMode = nullptr;
    HWND m_gPasteByDefault = nullptr;
    HWND m_gRemoveFormatting = nullptr;

    // Appearance page.
    HWND m_aPopupPosition = nullptr;
    HWND m_aPopupScreen = nullptr;
    HWND m_aPinTo = nullptr;
    HWND m_aImageHeight = nullptr;
    HWND m_aOpenPreview = nullptr;
    HWND m_aPreviewDelay = nullptr;
    HWND m_aHighlight = nullptr;
    HWND m_aMenuIcon = nullptr;
    HWND m_aShowStatus = nullptr;
    HWND m_aShowRecent = nullptr;
    HWND m_aShowSearch = nullptr;
    HWND m_aSearchVisibility = nullptr;
    HWND m_aShowSpecial = nullptr;
    HWND m_aShowTitle = nullptr;
    HWND m_aShowIcons = nullptr;
    HWND m_aShowSwatch = nullptr;
    HWND m_aShowFooter = nullptr;

    // Storage page.
    HWND m_sSaveFiles = nullptr;
    HWND m_sSaveImages = nullptr;
    HWND m_sSaveText = nullptr;
    HWND m_sHistorySize = nullptr;
    HWND m_sSortBy = nullptr;
    HWND m_sStorageSize = nullptr;

    // Ignore page. The three Maccy sub-tabs use one editor/list pair.
    CTabCtrl m_ignoreTabs;
    HWND m_ignoreTabWindow = nullptr;
    HWND m_iList = nullptr;
    HWND m_iEdit = nullptr;
    HWND m_iWhitelist = nullptr;
    HWND m_iDescription = nullptr;
    int m_ignorePage = 0;

    // Pins page.
    HWND m_pList = nullptr;
    HWND m_pKey = nullptr;
    HWND m_pTitle = nullptr;
    HWND m_pContent = nullptr;
    HWND m_pContentHint = nullptr;
    sqlite3_int64 m_selectedPinId = 0;
    bool m_selectedPinTextEditable = false;
    std::vector<ClipboardItem> m_pins;

    // Advanced page.
    HWND m_xIgnoreEvents = nullptr;
    HWND m_xIgnoreNext = nullptr;
    HWND m_xClearOnQuit = nullptr;
    HWND m_xClearClipboard = nullptr;
};
