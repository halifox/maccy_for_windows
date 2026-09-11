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
#include "resource.h"

constexpr UINT kSettingsChangedMessage = WM_APP + 20;

class SettingsWindow : public CDialogImpl<SettingsWindow> {
public:
    enum { IDD = IDD_SETTINGS };

    SettingsWindow(Database &database, HWND owner);

    bool CreateOrShow();
    void DestroyForOwner();
    bool IsOpen() const noexcept { return m_hWnd != nullptr && IsWindowVisible(); }
    HWND Window() const noexcept { return m_hWnd; }

    BEGIN_MSG_MAP(SettingsWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

private:
    static constexpr int kPageCount = 6;
    static constexpr int kIgnorePageCount = 3;

    void CreateTabs();
    bool CreatePageWindows();
    void BindControls();
    void PositionPages();

    void SetPage(int page);
    void SetIgnorePage(int page);
    RECT PageRect() const;
    RECT IgnorePageRect() const;

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
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);

    Database &m_database;
    HWND m_owner = nullptr;
    CTabCtrl m_tabs;
    std::array<HWND, kPageCount> m_pages{};
    std::array<HWND, kIgnorePageCount> m_ignorePages{};
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
    HWND m_aWindowWidth = nullptr;
    HWND m_aWindowHeight = nullptr;
    HWND m_aImageHeight = nullptr;
    HWND m_aOpenPreview = nullptr;
    HWND m_aPreviewDelay = nullptr;
    HWND m_aPreviewWidth = nullptr;
    HWND m_aHighlight = nullptr;
    HWND m_aMenuIcon = nullptr;
    HWND m_aShowStatus = nullptr;
    HWND m_aShowRecent = nullptr;
    HWND m_aShowSearch = nullptr;
    HWND m_aSearchVisibility = nullptr;
    HWND m_aShowTitle = nullptr;
    HWND m_aShowFooter = nullptr;
    HWND m_aShowSpecial = nullptr;
    HWND m_aShowIcons = nullptr;
    HWND m_aShowSwatch = nullptr;

    // Storage page.
    HWND m_sSaveFiles = nullptr;
    HWND m_sSaveImages = nullptr;
    HWND m_sSaveText = nullptr;
    HWND m_sHistorySize = nullptr;
    HWND m_sSortBy = nullptr;
    HWND m_sStorageSize = nullptr;

    // Ignore page. Each sub-tab is a separate resource dialog.
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
