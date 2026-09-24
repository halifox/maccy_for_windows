#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlddx.h>
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
    CComboBox m_keyCombo;
    CEdit m_titleEdit;
    CEdit m_contentEdit;
    CStatic m_hintLabel;
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
    CEdit m_valueEdit;
    CStatic m_descriptionLabel;
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

class SettingsResourcePageBase {
public:
    virtual ~SettingsResourcePageBase() = default;
    virtual HWND CreatePage(HWND parent, CMessageMap &owner) = 0;
    virtual HWND Handle() const noexcept = 0;
    virtual void LoadSettings(AppSettings &settings) = 0;
    virtual BOOL ExchangeSettings(BOOL saveAndValidate) = 0;

    void AttachSettings(AppSettings &settings) noexcept { m_settings = &settings; }

    CWindow Window() const noexcept { return CWindow(Handle()); }
    CWindow Control(int id) const noexcept { return Window().GetDlgItem(id); }

protected:
    AppSettings *m_settings = nullptr;
    int m_searchModeIndex = 0;
    int m_popupPositionIndex = 0;
    int m_popupScreenIndex = 0;
    int m_pinPositionIndex = 0;
    int m_highlightIndex = 0;
    int m_menuIconIndex = 0;
    int m_searchVisibilityIndex = 0;
    int m_sortByIndex = 0;
};

template <UINT ResourceId>
class SettingsResourcePage final :
    public CDialogImpl<SettingsResourcePage<ResourceId>>,
    public WTL::CWinDataExchange<SettingsResourcePage<ResourceId>>,
    public SettingsResourcePageBase {
public:
    enum { IDD = ResourceId };
    using WTL::CWinDataExchange<SettingsResourcePage<ResourceId>>::DDX_Check;
    using WTL::CWinDataExchange<SettingsResourcePage<ResourceId>>::DDX_Index;

    HWND CreatePage(HWND parent, CMessageMap &owner) override {
        m_ownerMessageMap = &owner;
        return CDialogImpl<SettingsResourcePage<ResourceId>>::Create(parent);
    }

    HWND Handle() const noexcept override { return this->m_hWnd; }

    void LoadSettings(AppSettings &settings) override {
        this->m_settings = &settings;
        if constexpr (ResourceId == IDD_PAGE_GENERAL) {
            this->m_searchModeIndex = static_cast<int>(settings.search_mode);
        } else if constexpr (ResourceId == IDD_PAGE_APPEARANCE) {
            this->m_popupPositionIndex = static_cast<int>(settings.popup_position);
            this->m_popupScreenIndex = std::clamp(
                settings.popup_screen, 0,
                std::max(1, static_cast<int>(::GetSystemMetrics(SM_CMONITORS))));
            this->m_pinPositionIndex = static_cast<int>(settings.pin_to);
            this->m_highlightIndex = static_cast<int>(settings.highlight_match);
            this->m_menuIconIndex = settings.menu_icon == L"clipboard" ? 1
                : settings.menu_icon == L"scissors" ? 2
                : settings.menu_icon == L"paperclip" ? 3 : 0;
            this->m_searchVisibilityIndex = static_cast<int>(settings.search_visibility);
        } else if constexpr (ResourceId == IDD_PAGE_STORAGE) {
            this->m_sortByIndex = settings.sort_by;
        }
    }

    BEGIN_DDX_MAP(SettingsResourcePage)
        if constexpr (ResourceId == IDD_PAGE_GENERAL) {
            DDX_CHECK(IDC_G_LAUNCH, this->m_settings->launch_at_login)
            DDX_CHECK(IDC_G_UPDATES, this->m_settings->check_for_updates)
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_G_SEARCH_MODE))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_G_SEARCH_MODE, this->m_searchModeIndex, bSaveAndValidate);
            DDX_CHECK(IDC_G_PASTE_BY_DEFAULT, this->m_settings->paste_by_default)
            DDX_CHECK(IDC_G_REMOVE_FORMATTING, this->m_settings->remove_formatting_by_default)
        } else if constexpr (ResourceId == IDD_PAGE_APPEARANCE) {
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_A_POPUP_POSITION))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_A_POPUP_POSITION, this->m_popupPositionIndex, bSaveAndValidate);
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_A_POPUP_SCREEN))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_A_POPUP_SCREEN, this->m_popupScreenIndex, bSaveAndValidate);
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_A_PIN_TO))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_A_PIN_TO, this->m_pinPositionIndex, bSaveAndValidate);
            DDX_CHECK(IDC_A_OPEN_PREVIEW, this->m_settings->open_preview_automatically)
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_A_HIGHLIGHT))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_A_HIGHLIGHT, this->m_highlightIndex, bSaveAndValidate);
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_A_MENU_ICON))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_A_MENU_ICON, this->m_menuIconIndex, bSaveAndValidate);
            DDX_CHECK(IDC_A_SHOW_STATUS, this->m_settings->show_in_status_bar)
            DDX_CHECK(IDC_A_SHOW_SEARCH, this->m_settings->show_search)
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_A_SEARCH_VISIBILITY))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_A_SEARCH_VISIBILITY, this->m_searchVisibilityIndex, bSaveAndValidate);
            DDX_CHECK(IDC_A_SHOW_TITLE, this->m_settings->show_title)
            DDX_CHECK(IDC_A_SHOW_FOOTER, this->m_settings->show_footer)
            DDX_CHECK(IDC_A_SHOW_SPECIAL, this->m_settings->show_special_symbols)
            DDX_CHECK(IDC_A_SHOW_ICONS, this->m_settings->show_application_icons)
            DDX_CHECK(IDC_A_SHOW_SWATCH, this->m_settings->show_hex_color_swatch)
        } else if constexpr (ResourceId == IDD_PAGE_STORAGE) {
            DDX_CHECK(IDC_S_SAVE_FILES, this->m_settings->save_files)
            DDX_CHECK(IDC_S_SAVE_IMAGES, this->m_settings->save_images)
            DDX_CHECK(IDC_S_SAVE_TEXT, this->m_settings->save_text)
            if ((nCtlID == (UINT)-1) || (nCtlID == IDC_S_SORT_BY))
                this->template DDX_Index<WTL::CComboBox>(
                    IDC_S_SORT_BY, this->m_sortByIndex, bSaveAndValidate);
        } else if constexpr (ResourceId == IDD_IGNORE_APPLICATIONS) {
            DDX_CHECK(IDC_I_WHITELIST, this->m_settings->ignore_all_apps_except_listed)
        } else if constexpr (ResourceId == IDD_PAGE_ADVANCED) {
            DDX_CHECK(IDC_X_IGNORE_EVENTS, this->m_settings->ignore_events)
            DDX_CHECK(IDC_X_IGNORE_NEXT, this->m_settings->ignore_only_next_event)
            DDX_CHECK(IDC_X_CLEAR_ON_QUIT, this->m_settings->clear_on_quit)
            DDX_CHECK(IDC_X_CLEAR_CLIPBOARD, this->m_settings->clear_system_clipboard)
            DDX_CHECK(IDC_X_RESPECT_WINDOWS_CLIPBOARD_HISTORY,
                      this->m_settings->respect_windows_clipboard_history_markers)
        }
    END_DDX_MAP()

    BOOL ExchangeSettings(BOOL saveAndValidate) override {
        if (this->m_settings == nullptr || !DoDataExchange(saveAndValidate)) {
            return FALSE;
        }
        if (!saveAndValidate) {
            return TRUE;
        }

        if constexpr (ResourceId == IDD_PAGE_GENERAL) {
            this->m_settings->search_mode = static_cast<SearchMode>(
                std::clamp(this->m_searchModeIndex, 0, 3));
        } else if constexpr (ResourceId == IDD_PAGE_APPEARANCE) {
            this->m_settings->popup_position = static_cast<PopupPosition>(
                std::max(0, this->m_popupPositionIndex));
            this->m_settings->popup_screen = std::max(0, this->m_popupScreenIndex);
            this->m_settings->pin_to = static_cast<PinPosition>(
                std::max(0, this->m_pinPositionIndex));
            this->m_settings->highlight_match = static_cast<HighlightMatch>(
                std::max(0, this->m_highlightIndex));
            constexpr std::array<const wchar_t *, 4> icons = {
                L"maccy", L"clipboard", L"scissors", L"paperclip"};
            this->m_settings->menu_icon = icons[static_cast<size_t>(
                std::clamp(this->m_menuIconIndex, 0, 3))];
            this->m_settings->search_visibility = static_cast<SearchVisibility>(
                std::max(0, this->m_searchVisibilityIndex));
        } else if constexpr (ResourceId == IDD_PAGE_STORAGE) {
            this->m_settings->sort_by = std::clamp(this->m_sortByIndex, 0, 2);
        }
        return TRUE;
    }

    BEGIN_MSG_MAP(SettingsResourcePage)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_COMMAND, OnOwnerMessage)
        MESSAGE_HANDLER(WM_NOTIFY, OnOwnerMessage)
    END_MSG_MAP()

private:
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
        handled = TRUE;
        return TRUE;
    }

    LRESULT OnOwnerMessage(UINT message, WPARAM wParam, LPARAM lParam, BOOL &handled) {
        if (m_ownerMessageMap == nullptr) {
            handled = FALSE;
            return 0;
        }

        LRESULT result = 0;
        m_ownerMessageMap->ProcessWindowMessage(
            this->m_hWnd, message, wParam, lParam, result, 0);
        // Resource-page controls have no default dialog behavior for these
        // notifications. Match the previous page forwarding while dispatching
        // through the owner's WTL message map instead of SendMessage.
        handled = TRUE;
        return result;
    }

    CMessageMap *m_ownerMessageMap = nullptr;
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
    CWindow m_owner;
    SettingsChangedCallback m_onChanged;
    UpdateCheckCallback m_onUpdateCheck;
    CTabCtrl m_tabs;
    std::array<std::unique_ptr<SettingsResourcePageBase>, AppConstants::SettingsUI::kPageCount> m_pages{
        std::make_unique<SettingsResourcePage<IDD_PAGE_GENERAL>>(),
        std::make_unique<SettingsResourcePage<IDD_PAGE_STORAGE>>(),
        std::make_unique<SettingsResourcePage<IDD_PAGE_APPEARANCE>>(),
        std::make_unique<SettingsResourcePage<IDD_PAGE_PINS>>(),
        std::make_unique<SettingsResourcePage<IDD_PAGE_IGNORE>>(),
        std::make_unique<SettingsResourcePage<IDD_PAGE_ADVANCED>>()
    };
    std::array<std::unique_ptr<SettingsResourcePageBase>, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePages{
        std::make_unique<SettingsResourcePage<IDD_IGNORE_APPLICATIONS>>(),
        std::make_unique<SettingsResourcePage<IDD_IGNORE_FORMATS>>(),
        std::make_unique<SettingsResourcePage<IDD_IGNORE_REGEXPS>>()
    };
    int m_currentPage = 0;
    bool m_loading = false;
    bool m_destroying = false;
    bool m_updateCheckBusy = false;
    HICON m_windowIcon = nullptr;

    AppSettings m_settings{};
    std::array<std::vector<std::wstring>, 3> m_ignoredLists;

    CButton m_gCheckNow;
    CStatic m_gBehaviorHint;
    CHotKeyCtrl m_gOpenHotKey;
    CHotKeyCtrl m_gPinHotKey;
    CHotKeyCtrl m_gDeleteHotKey;
    CHotKeyCtrl m_gPreviewHotKey;
    CButton m_gPasteByDefault;
    CButton m_gRemoveFormatting;

    CComboBox m_aPopupPosition;
    CButton m_aResetPosition;
    CEdit m_aImageHeight;
    CButton m_aOpenPreview;
    CEdit m_aPreviewDelay;
    CComboBox m_aMenuIcon;
    CButton m_aShowStatus;
    CButton m_aShowSearch;
    CComboBox m_aSearchVisibility;
    CButton m_aShowTitle;

    CEdit m_sHistorySize;
    CStatic m_sStorageSize;
    CStatic m_sCurrentSize;

    CTabCtrl m_ignoreTabs;
    std::array<std::unique_ptr<IgnorePageBase>, AppConstants::SettingsUI::kIgnorePageCount> m_ignorePageObjects;
    CImageListManaged m_ignoreImageList;
    int m_ignorePage = 0;

    CListViewCtrl m_pList;
    std::vector<ClipboardItem> m_pins;

    CButton m_xIgnoreEvents;
    CButton m_xIgnoreNext;
};
