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

// A page is a real child window of the tab control.  Keeping its semantic
// layout and scroll state here prevents the settings window from mixing
// tab-local and parent-window coordinates.  Pages describe blocks and rows;
// this class measures and positions the native Win32 controls.
class SettingsPageWindow : public CWindowImpl<SettingsPageWindow> {
public:
    DECLARE_WND_CLASS_EX(L"ClipboardSettingsPage", CS_HREDRAW | CS_VREDRAW, COLOR_BTNFACE)

    struct LayoutOptions {
        // Widths and heights are expressed in 96-DPI logical pixels.  A zero
        // width means that the layout engine measures the control's content.
        int width = 0;
        int minimum_height = 0;
        bool fill_width = false;
        bool fill_height = false;
        bool label = false;
        bool section = false;
        bool combo = false;
    };

    struct LayoutCell {
        HWND window = nullptr;
        LayoutOptions options{};
    };

    struct LayoutItem {
        enum class Kind {
            Block,
            Row,
        };

        Kind kind = Kind::Block;
        HWND window = nullptr;
        LayoutOptions options{};
        std::vector<LayoutCell> cells;
        int gap = 8;
    };

    void Configure(HWND owner, bool scrollable, int minimum_content_height);
    void BeginRow(int gap = 8);
    void EndRow();
    void AddLayout(HWND window, LayoutOptions options = {});
    void AddSpacer(LayoutOptions options = {});
    void SetContentSize(int minimum_content_height);
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
    bool IsExplicitlyVisible(HWND window) const noexcept;
    int MeasureTextWidth(HWND window) const;
    int MeasureTextHeight(HWND window, int width) const;
    int MeasureCellWidth(const LayoutCell &cell) const;
    int MeasureCellHeight(const LayoutCell &cell, int width) const;

    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnVScroll(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnMouseWheel(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);

    HWND m_owner = nullptr;
    std::vector<LayoutItem> m_layout;
    std::vector<LayoutCell> m_activeRow;
    int m_activeRowGap = 8;
    bool m_rowOpen = false;
    int m_minimumContentHeight = 0;
    int m_contentHeight = 0;
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

    using LayoutOptions = SettingsPageWindow::LayoutOptions;

    void BeginRow(int page, int gap = 8);
    void EndRow(int page);
    HWND AddStatic(
        int page,
        const wchar_t *text,
        DWORD style = SS_LEFT,
        LayoutOptions options = {}
    );
    HWND AddSectionHeading(int page, const wchar_t *text);
    HWND AddButton(
        int page,
        const wchar_t *text,
        int id,
        DWORD style = BS_PUSHBUTTON,
        LayoutOptions options = {}
    );
    HWND AddCheckBox(
        int page,
        const wchar_t *text,
        int id,
        LayoutOptions options = {}
    );
    HWND AddEdit(int page, int id, DWORD style = ES_AUTOHSCROLL, LayoutOptions options = {});
    HWND AddCombo(int page, int id, LayoutOptions options = {});
    HWND AddList(int page, int id, LayoutOptions options = {});
    HWND AddHotKey(int page, int id, LayoutOptions options = {});
    void AddLayout(
        int page,
        HWND window,
        LayoutOptions options = {}
    );
    void AddSpacer(int page, LayoutOptions options = {});

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
