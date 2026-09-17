#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

#include "Settings.h"
#include "Database.h"
#include "resource.h"

class EditPinDialog : public CDialogImpl<EditPinDialog> {
public:
    enum { IDD = IDD_EDIT_PIN };

    EditPinDialog(const AppSettings &settings, sqlite3_int64 item_id,
                  const std::vector<ClipboardItem> &pins, ClipboardItem item);

    std::wstring GetKey() const { return m_key; }
    std::wstring GetTitle() const { return m_title; }
    std::wstring GetContent() const { return m_content; }
    sqlite3_int64 GetItemId() const noexcept { return m_itemId; }
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

    const AppSettings &m_settings;
    sqlite3_int64 m_itemId;
    const std::vector<ClipboardItem> &m_pins;
    ClipboardItem m_item;
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

class IgnorePageBase {
public:
    virtual ~IgnorePageBase() = default;

    virtual void Initialize(HWND page_window, Database &database,
                            std::vector<std::wstring> values) = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void Refresh() = 0;
    virtual bool AddValue() = 0;
    virtual bool EditValue() = 0;
    virtual bool RemoveValue() = 0;
    virtual bool ResetToDefaults() = 0;
    virtual bool SaveList() = 0;

    HWND GetPageWindow() const { return m_pageWindow; }
    const std::vector<std::wstring> &Values() const noexcept { return m_values; }

protected:
    HWND m_pageWindow = nullptr;
    HWND m_list = nullptr;
    HWND m_description = nullptr;
    Database *m_database = nullptr;
    std::vector<std::wstring> m_values;
};

class IgnoreApplicationsPage : public IgnorePageBase {
public:
    void Initialize(HWND page_window, Database &database,
                    std::vector<std::wstring> values) override;
    void Show() override;
    void Hide() override;
    void Refresh() override;
    bool AddValue() override;
    bool EditValue() override;
    bool RemoveValue() override;
    bool ResetToDefaults() override;
    bool SaveList() override;

private:
    void UpdateDescription();
};

class IgnoreFormatsPage : public IgnorePageBase {
public:
    void Initialize(HWND page_window, Database &database,
                    std::vector<std::wstring> values) override;
    void Show() override;
    void Hide() override;
    void Refresh() override;
    bool AddValue() override;
    bool EditValue() override;
    bool RemoveValue() override;
    bool ResetToDefaults() override;
    bool SaveList() override;

private:
    void UpdateDescription();
};

class IgnoreRegexpsPage : public IgnorePageBase {
public:
    void Initialize(HWND page_window, Database &database,
                    std::vector<std::wstring> values) override;
    void Show() override;
    void Hide() override;
    void Refresh() override;
    bool AddValue() override;
    bool EditValue() override;
    bool RemoveValue() override;
    bool ResetToDefaults() override;
    bool SaveList() override;

private:
    void UpdateDescription();
};

class SettingsWindow : public CDialogImpl<SettingsWindow> {
public:
    enum { IDD = IDD_SETTINGS };

    using SettingsChangedCallback = std::function<void(const AppSettings &, std::uint32_t)>;
    SettingsWindow(Database &database, HWND owner, AppSettings settings,
                   std::array<std::vector<std::wstring>, 3> ignored_lists,
                   SettingsChangedCallback on_changed);

    bool CreateOrShow();
    void DestroyForOwner();
    void SetSettingsSnapshot(const AppSettings &settings);
    bool IsOpen() const noexcept { return m_hWnd != nullptr && IsWindowVisible(); }
    HWND Window() const noexcept { return m_hWnd; }

    BEGIN_MSG_MAP(SettingsWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

private:
    void CreateTabs();
    bool CreatePageWindows();
    void BindControls();
    void ConfigureIgnoreList();
    void ConfigurePinsList();

    void SetPage(int page);
    void SetIgnorePage(int page);

    void LoadControlsFromSettings();
    void LoadGeneralControls();
    void LoadAppearanceControls();
    void LoadStorageControls();
    void LoadAdvancedControls();
    void UpdateDependencies();
    void RefreshPinsList();

    void SaveCurrentPage();
    void NotifyOwner(std::uint32_t updateMask = AppConstants::UiUpdate::kSettings);
    void EditSelectedPin();
    void DeleteSelectedPin();
    void OpenNotificationsSettings();
    void CheckForUpdatesNow();
    void ResetPopupPosition();

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCommand(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnNotify(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);

    Database &m_database;
    HWND m_owner = nullptr;
    SettingsChangedCallback m_onChanged;
    CTabCtrl m_tabs;
    std::array<HWND, AppConstants::SettingsUI::kPageCount> m_pages{};
    std::array<HWND, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePages{};
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_destroying = false;
    HICON m_windowIcon = nullptr;

    AppSettings m_settings{};
    std::array<std::vector<std::wstring>, 3> m_ignoredLists;

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
    HWND m_xRespectWindowsClipboardHistory = nullptr;
};
