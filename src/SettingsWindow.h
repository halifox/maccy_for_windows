#pragma once

#define NOMINMAX
#include <windows.h>

#include <array>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

#include "Database.h"
#include "Settings.h"

constexpr UINT kSettingsChangedMessage = WM_APP + 20;

class SettingsWindow : public CWindowImpl<SettingsWindow> {
public:
    SettingsWindow(Database &database, HWND owner);

    DECLARE_WND_CLASS_EX(L"ClipboardSettingsWindow", CS_HREDRAW | CS_VREDRAW, COLOR_BTNFACE)

    bool CreateOrShow();
    void DestroyForOwner();
    bool IsOpen() const noexcept { return m_hWnd != nullptr && IsWindowVisible(); }
    HWND Window() const noexcept { return m_hWnd; }

    BEGIN_MSG_MAP(SettingsWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

private:
    struct LayoutControl {
        HWND window = nullptr;
        RECT relative{};
        bool stretch_width = false;
        bool stretch_height = false;
    };

    static constexpr int kPageCount = 6;

    HWND AddStatic(int page, const wchar_t *text, RECT relative, DWORD style = SS_LEFT);
    HWND AddButton(int page, const wchar_t *text, int id, RECT relative, DWORD style = BS_PUSHBUTTON);
    HWND AddCheckBox(int page, const wchar_t *text, int id, RECT relative);
    HWND AddEdit(int page, int id, RECT relative, DWORD style = ES_AUTOHSCROLL);
    HWND AddCombo(int page, int id, RECT relative);
    HWND AddList(int page, int id, RECT relative, bool stretch_height = true);
    HWND AddHotKey(int page, int id, RECT relative);
    void AddLayout(int page, HWND window, RECT relative, bool stretch_width = false, bool stretch_height = false);

    void CreateTabs();
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

    LRESULT OnCreate(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);

    Database &m_database;
    HWND m_owner = nullptr;
    CTabCtrl m_tabs;
    HWND m_tabWindow = nullptr;
    std::array<std::vector<LayoutControl>, kPageCount> m_pageControls;
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_destroying = false;

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

    // Ignore page. The three Maccy sub-tabs use one small editor/list pair.
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
