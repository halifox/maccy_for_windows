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
#include "ClipboardData.h"
#include "StorageWorker.h"
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

    virtual void Initialize(CWindow page_window, StorageWorker &storage,
                            std::vector<std::wstring> values) = 0;
    virtual void Show() = 0;
    virtual void Hide() = 0;
    virtual void Refresh() = 0;
    virtual bool AddValue() = 0;
    virtual bool EditValue() = 0;
    virtual bool RemoveValue() = 0;
    virtual bool ResetToDefaults() = 0;
    virtual bool SaveList() = 0;

    CWindow GetPageWindow() const { return m_pageWindow; }
    CListViewCtrl ListWindow() const { return m_list; }
    const std::vector<std::wstring> &Values() const noexcept { return m_values; }
    void SetValues(std::vector<std::wstring> values);

protected:
    bool PersistValues(IgnoreListKind list);

    CWindow m_pageWindow;
    CListViewCtrl m_list;
    CStatic m_description;
    StorageWorker *m_storage = nullptr;
    std::vector<std::wstring> m_values;
    std::vector<std::wstring> m_persistedValues;
};

class IgnoreApplicationsPage : public IgnorePageBase {
public:
    void Initialize(CWindow page_window, StorageWorker &storage,
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
    void Initialize(CWindow page_window, StorageWorker &storage,
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
    void Initialize(CWindow page_window, StorageWorker &storage,
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
    using UpdateCheckCallback = std::function<bool()>;
    SettingsWindow(StorageWorker &storage, HWND owner, AppSettings settings,
                   std::array<std::vector<std::wstring>, 3> ignored_lists,
                   SettingsChangedCallback on_changed,
                   UpdateCheckCallback on_update_check);

    bool CreateOrShow();
    void DestroyForOwner();
    void SetSettingsSnapshot(const AppSettings &settings);
    void SetStateSnapshot(const AppSettings &settings, StorageWorker::IgnoreLists ignored_lists);
    void SetUpdateCheckBusy(bool busy);
    bool IsOpen() const noexcept { return m_hWnd != nullptr && IsWindowVisible(); }
    HWND Window() const noexcept { return m_hWnd; }

    BEGIN_MSG_MAP(SettingsWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)

        COMMAND_HANDLER(IDC_G_CHECK_NOW, BN_CLICKED, OnCheckUpdatesCommand)
        COMMAND_HANDLER(IDC_G_NOTIFICATIONS, BN_CLICKED, OnNotificationsCommand)
        COMMAND_HANDLER(IDC_A_RESET_POSITION, BN_CLICKED, OnResetPositionCommand)
        COMMAND_HANDLER(IDC_I_ADD, BN_CLICKED, OnIgnorePageCommand)
        COMMAND_HANDLER(IDC_I_REMOVE, BN_CLICKED, OnIgnorePageCommand)
        COMMAND_HANDLER(IDC_I_RESET, BN_CLICKED, OnIgnorePageCommand)

        COMMAND_HANDLER(IDC_G_LAUNCH, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_UPDATES, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_OPEN_HOTKEY, EN_CHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_PIN_HOTKEY, EN_CHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_DELETE_HOTKEY, EN_CHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_PREVIEW_HOTKEY, EN_CHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_SEARCH_MODE, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_PASTE_BY_DEFAULT, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_G_REMOVE_FORMATTING, BN_CLICKED, OnSettingChanged)

        COMMAND_HANDLER(IDC_A_POPUP_POSITION, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_POPUP_SCREEN, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_PIN_TO, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_IMAGE_HEIGHT, EN_KILLFOCUS, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_OPEN_PREVIEW, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_PREVIEW_DELAY, EN_KILLFOCUS, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_HIGHLIGHT, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_MENU_ICON, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_STATUS, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_SEARCH, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SEARCH_VISIBILITY, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_TITLE, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_FOOTER, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_SPECIAL, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_ICONS, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_A_SHOW_SWATCH, BN_CLICKED, OnSettingChanged)

        COMMAND_HANDLER(IDC_S_SAVE_FILES, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_S_SAVE_IMAGES, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_S_SAVE_TEXT, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_S_HISTORY_SIZE, EN_KILLFOCUS, OnSettingChanged)
        COMMAND_HANDLER(IDC_S_SORT_BY, CBN_SELCHANGE, OnSettingChanged)
        COMMAND_HANDLER(IDC_I_WHITELIST, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_X_IGNORE_EVENTS, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_X_IGNORE_NEXT, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_X_CLEAR_ON_QUIT, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_X_CLEAR_CLIPBOARD, BN_CLICKED, OnSettingChanged)
        COMMAND_HANDLER(IDC_X_RESPECT_WINDOWS_CLIPBOARD_HISTORY, BN_CLICKED, OnSettingChanged)

        NOTIFY_HANDLER(IDC_SETTINGS_TABS, TCN_SELCHANGE, OnTabsSelectionChanged)
        NOTIFY_HANDLER(IDC_IGNORE_TABS, TCN_SELCHANGE, OnIgnoreTabsSelectionChanged)
        NOTIFY_HANDLER(IDC_I_LIST, NM_DBLCLK, OnIgnoreListDoubleClick)
        NOTIFY_HANDLER(IDC_I_LIST, LVN_KEYDOWN, OnIgnoreListKeyDown)
        NOTIFY_HANDLER(IDC_P_LIST, NM_DBLCLK, OnPinsListDoubleClick)
        NOTIFY_HANDLER(IDC_P_LIST, LVN_KEYDOWN, OnPinsListKeyDown)
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
    void NotifyOwner(std::uint32_t updateMask = 0);
    void EditSelectedPin();
    void DeleteSelectedPin();
    void OpenNotificationsSettings();
    void CheckForUpdatesNow();
    void ResetPopupPosition();

    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnDpiChanged(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL &handled);
    LRESULT OnCheckUpdatesCommand(WORD, WORD, HWND, BOOL &handled);
    LRESULT OnNotificationsCommand(WORD, WORD, HWND, BOOL &handled);
    LRESULT OnResetPositionCommand(WORD, WORD, HWND, BOOL &handled);
    LRESULT OnIgnorePageCommand(WORD, WORD id, HWND, BOOL &handled);
    LRESULT OnSettingChanged(WORD, WORD id, HWND, BOOL &handled);
    LRESULT OnTabsSelectionChanged(int, LPNMHDR, BOOL &handled);
    LRESULT OnIgnoreTabsSelectionChanged(int, LPNMHDR, BOOL &handled);
    LRESULT OnIgnoreListDoubleClick(int, LPNMHDR, BOOL &handled);
    LRESULT OnIgnoreListKeyDown(int, LPNMHDR, BOOL &handled);
    LRESULT OnPinsListDoubleClick(int, LPNMHDR, BOOL &handled);
    LRESULT OnPinsListKeyDown(int, LPNMHDR, BOOL &handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled);

    StorageWorker &m_storage;
    HWND m_owner = nullptr;
    SettingsChangedCallback m_onChanged;
    UpdateCheckCallback m_onUpdateCheck;
    CTabCtrl m_tabs;
    std::array<CWindow, AppConstants::SettingsUI::kPageCount> m_pages{};
    std::array<CWindow, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePages{};
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_destroying = false;
    bool m_updateCheckBusy = false;
    HICON m_windowIcon = nullptr;

    AppSettings m_settings{};
    std::array<std::vector<std::wstring>, 3> m_ignoredLists;

    CButton m_gLaunch;
    CButton m_gUpdates;
    CButton m_gCheckNow;
    CStatic m_gBehaviorHint;
    CHotKeyCtrl m_gOpenHotKey;
    CHotKeyCtrl m_gPinHotKey;
    CHotKeyCtrl m_gDeleteHotKey;
    CHotKeyCtrl m_gPreviewHotKey;
    CComboBox m_gSearchMode;
    CButton m_gPasteByDefault;
    CButton m_gRemoveFormatting;

    CComboBox m_aPopupPosition;
    CComboBox m_aPopupScreen;
    CButton m_aResetPosition;
    CComboBox m_aPinTo;
    CEdit m_aImageHeight;
    CButton m_aOpenPreview;
    CEdit m_aPreviewDelay;
    CComboBox m_aHighlight;
    CComboBox m_aMenuIcon;
    CButton m_aShowStatus;
    CButton m_aShowSearch;
    CComboBox m_aSearchVisibility;
    CButton m_aShowTitle;
    CButton m_aShowFooter;
    CButton m_aShowSpecial;
    CButton m_aShowIcons;
    CButton m_aShowSwatch;

    CButton m_sSaveFiles;
    CButton m_sSaveImages;
    CButton m_sSaveText;
    CEdit m_sHistorySize;
    CComboBox m_sSortBy;
    CStatic m_sStorageSize;
    CStatic m_sCurrentSize;

    CTabCtrl m_ignoreTabs;
    std::array<std::unique_ptr<IgnorePageBase>, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePageObjects;
    CImageListManaged m_ignoreImageList;
    int m_ignorePage = 0;

    CListViewCtrl m_pList;
    CButton m_iWhitelist;
    std::vector<ClipboardItem> m_pins;

    CButton m_xIgnoreEvents;
    CButton m_xIgnoreNext;
    CButton m_xClearOnQuit;
    CButton m_xClearClipboard;
    CButton m_xRespectWindowsClipboardHistory;
};
