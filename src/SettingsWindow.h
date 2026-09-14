#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

#include "Database.h"
#include "Settings.h"
#include "resource.h"
#include "IgnorePages.h"

class EditPinDialog : public CDialogImpl<EditPinDialog> {
public:
    enum { IDD = IDD_EDIT_PIN };

    EditPinDialog(Database &database, const AppSettings &settings, sqlite3_int64 item_id, const std::vector<ClipboardItem> &pins);

    std::wstring GetKey() const { return m_key; }
    std::wstring GetTitle() const { return m_title; }
    std::wstring GetContent() const { return m_content; }
    bool ContentModified() const { return m_contentModified; }

    BEGIN_MSG_MAP(EditPinDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

private:
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnOK(WORD, WORD, HWND, BOOL &handled);
    LRESULT OnCancel(WORD, WORD, HWND, BOOL &handled);

    Database &m_database;
    const AppSettings &m_settings;
    sqlite3_int64 m_itemId;
    const std::vector<ClipboardItem> &m_pins;
    std::wstring m_key;
    std::wstring m_title;
    std::wstring m_content;
    std::wstring m_originalContent;
    bool m_contentModified = false;
    bool m_textEditable = false;
};

class EditIgnoreDialog : public CDialogImpl<EditIgnoreDialog> {
public:
    enum { IDD = IDD_EDIT_IGNORE };

    EditIgnoreDialog(const std::wstring &value, const std::wstring &description, int ignore_page);

    std::wstring GetValue() const { return m_value; }

    BEGIN_MSG_MAP(EditIgnoreDialog)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

private:
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnOK(WORD, WORD, HWND, BOOL &handled);
    LRESULT OnCancel(WORD, WORD, HWND, BOOL &handled);

    std::wstring m_value;
    std::wstring m_description;
    int m_ignorePage;
};

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
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

private:
    void CreateTabs();
    bool CreatePageWindows();
    void BindControls();
    void PositionPages();
    void EnsureCurrentPageFits();
    void LayoutFlexibleControls();
    void ConfigureIgnoreList();
    void ConfigurePinsList();

    void SetPage(int page);
    void SetIgnorePage(int page);
    RECT PageRect() const;
    RECT IgnorePageRect() const;

    void LoadControlsFromSettings();
    void LoadGeneralControls();
    void LoadAppearanceControls();
    void LoadStorageControls();
    void LoadAdvancedControls();
    void UpdateDependencies();
    void RefreshPinsList();

    void SaveCurrentPage(bool notify = true);
    void NotifyOwner();
    void EditSelectedPin();
    void DeleteSelectedPin();
    void OpenNotificationsSettings();
    void CheckForUpdatesNow();
    void ResetPopupPosition();

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);

    Database &m_database;
    HWND m_owner = nullptr;
    CTabCtrl m_tabs;
    std::array<HWND, AppConstants::SettingsUI::kPageCount> m_pages{};
    std::array<HWND, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePages{};
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_destroying = false;

    AppSettings m_settings{};

    HWND m_gLaunch = nullptr;
    HWND m_gUpdates = nullptr;
    HWND m_gOpenHotKey = nullptr;
    HWND m_gPinHotKey = nullptr;
    HWND m_gDeleteHotKey = nullptr;
    HWND m_gPreviewHotKey = nullptr;
    HWND m_gSearchMode = nullptr;
    HWND m_gPasteByDefault = nullptr;
    HWND m_gRemoveFormatting = nullptr;

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
    HWND m_aShowTitle = nullptr;
    HWND m_aShowFooter = nullptr;
    HWND m_aShowSpecial = nullptr;
    HWND m_aShowIcons = nullptr;
    HWND m_aShowSwatch = nullptr;

    HWND m_sSaveFiles = nullptr;
    HWND m_sSaveImages = nullptr;
    HWND m_sSaveText = nullptr;
    HWND m_sHistorySize = nullptr;
    HWND m_sSortBy = nullptr;
    HWND m_sStorageSize = nullptr;
    HWND m_sCurrentSize = nullptr;

    CTabCtrl m_ignoreTabs;
    std::array<std::unique_ptr<IgnorePageBase>, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePageObjects;
    HIMAGELIST m_ignoreImageList = nullptr;
    int m_ignorePage = 0;

    HWND m_pList = nullptr;
    std::vector<ClipboardItem> m_pins;

    HWND m_xIgnoreEvents = nullptr;
    HWND m_xIgnoreNext = nullptr;
    HWND m_xClearOnQuit = nullptr;
    HWND m_xClearClipboard = nullptr;
};
