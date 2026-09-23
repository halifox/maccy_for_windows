#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlwin.h>

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "HistoryRenderer.h"
#include "KeyboardHandler.h"
#include "PasteController.h"
#include "PreviewWorker.h"
#include "SearchHeaderLayout.h"
#include "Settings.h"
#include "ClipboardMonitor.h"
#include "ClipboardData.h"
#include "StorageWorker.h"
#include "UpdateChecker.h"
#include "resource.h"

class SettingsWindow;

// Main application window - coordinates all components
class MainWindow : public CDialogImpl<MainWindow> {
public:
    enum { IDD = IDD_HISTORY };

    MainWindow(StorageWorker &storage, PreviewWorker &preview, AppSettings settings,
               StorageWorker::IgnoreLists ignored_lists, bool isolated = false);
    ~MainWindow();

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnSearchEditColor)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(AppConstants::kPopupActivationMessage, OnPopupActivation)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_HOTKEY, OnHotKey)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_SYSKEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_SYSKEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_IME_STARTCOMPOSITION, OnImeStart)
        MESSAGE_HANDLER(WM_IME_ENDCOMPOSITION, OnImeEnd)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(AppConstants::kTrayIconMessage, OnTrayIcon)
        MESSAGE_HANDLER(AppConstants::kUiUpdateMessage, OnUiUpdate)
        MESSAGE_HANDLER(WM_CLIPBOARDUPDATE, OnClipboardUpdate)
        MESSAGE_HANDLER(AppConstants::kPreviewWorkerResultMessage, OnPreviewWorkerResult)
        MESSAGE_HANDLER(AppConstants::kStorageWorkerResultMessage, OnStorageWorkerResult)
        MESSAGE_HANDLER(AppConstants::kUpdateCheckerResultMessage, OnUpdateCheckerResult)
        COMMAND_HANDLER(IDC_HISTORY_SEARCH, EN_CHANGE, OnSearchChanged)
        COMMAND_HANDLER(IDC_HISTORY_LIST, LBN_SELCHANGE, OnHistoryListSelectionChanged)
        COMMAND_HANDLER(IDC_HISTORY_PINS, LBN_SELCHANGE, OnHistoryListSelectionChanged)
        COMMAND_HANDLER(IDC_HISTORY_CLEAR, BN_CLICKED, OnClearHistoryButton)
        COMMAND_HANDLER(IDC_HISTORY_SETTINGS, BN_CLICKED, OnSettingsButton)
        COMMAND_HANDLER(IDC_HISTORY_ABOUT, BN_CLICKED, OnAboutButton)
        COMMAND_HANDLER(IDC_HISTORY_PREVIEW, BN_CLICKED, OnPreviewToggleButton)
        COMMAND_HANDLER(IDC_HISTORY_EXIT, BN_CLICKED, OnExitButton)
        COMMAND_ID_HANDLER(kTrayCommandShow, OnTrayShowCommand)
        COMMAND_ID_HANDLER(kTrayCommandSettings, OnTraySettingsCommand)
        COMMAND_ID_HANDLER(kTrayCommandClear, OnTrayClearCommand)
        COMMAND_ID_HANDLER(kTrayCommandIgnore, OnTrayIgnoreCommand)
        COMMAND_ID_HANDLER(kTrayCommandExit, OnTrayExitCommand)
        // Preview actions carry an item ID in lParam, so this final WM_COMMAND
        // handler is retained for that nonstandard notification payload.
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
    END_MSG_MAP()

    bool AddTrayIcon();
    void ShowMainWindow();
    void ExitForInstaller();
    HWND Window() const noexcept { return m_hWnd; }

private:
    enum : UINT {
        kTrayCommandShow = 1001,
        kTrayCommandSettings = 1002,
        kTrayCommandClear = 1003,
        kTrayCommandIgnore = 1004,
        kTrayCommandExit = 1005
    };

    enum class ExitReason {
        User,
        Installer
    };

    // Window procedure callbacks for subclassed controls
    static LRESULT CALLBACK SearchWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HistoryListWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MenuControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR, DWORD_PTR data);

    // Search edit rendering
    void DrawSearchCue(HWND window, HDC dc) const;
    bool IsSearchClearHit(POINT point) const;
    void ClearSearch();

    // Control management
    bool BindControls();
    void ApplyHistoryFonts();
    void LayoutHistoryControls();
    void RedrawHistoryLists();
    void RedrawFooterButtons();
    void RestoreControlSubclass(HWND control, WNDPROC original);
    void RestoreControlSubclasses();
    std::array<CButton, AppConstants::UI::kFooterButtonCount> FooterButtons() const;

    // Window positioning
    void PositionPopup(PopupPosition popup_position);
    HMONITOR SelectedMonitor() const;
    int PopupWidth() const;
    int PopupHeight() const;

    // History management
    void RefreshHistory(std::wstring_view query);
    void ApplyHistoryItems(std::wstring query, std::vector<ClipboardItem> items);
    void ApplyDeferredHistoryResult();
    void ApplyHistoryVisibility();
    void SetHistorySearchVisible(bool visible);

    // Preview window
    void SchedulePreviewForItem(sqlite3_int64 item_id);
    void ShowPreviewForItem(sqlite3_int64 item_id);
    void ShowPreviewForCandidate();
    void ShowPreviewForSelection();
    void HidePreview();
    void TogglePreview();

    // Actions
    void ShowMainWindow(PopupPosition popup_position);
    void PasteItem(int index);
    void PasteSelectedItem();
    void ToggleSelectedPin();
    void DeleteSelectedItem();
    void ClearHistory(bool all = false);
    void OpenAbout();
    void OpenSettings();
    void ExitApplication(ExitReason reason = ExitReason::User);

    // Utilities
    void UpdateFooterControls();
    void RequestFooterUpdateForKeyMessage(UINT message, WPARAM key);
    void ScheduleSearchFromCurrentEdit();
    void SaveWindowGeometry(bool resized = false);
    void HideMainWindow();
    int SelectedHistoryIndex() const;
    bool IsOurWindow(HWND window) const;
    void HandlePopupActivation(HWND activating_window);

    // Tray icon
    void UpdateTrayTooltip();
    void UpdateTrayIcon();
    void ShowTrayMenu();
    void RemoveTrayIcon();

    // Settings
    std::uint32_t ApplySettings(const AppSettings &settings, std::uint32_t requestedUpdates);
    void PersistSettings();
    void RequestUiUpdate(std::uint32_t updateMask);
    void ApplyPendingState();
    void OnSettingsChanged(const AppSettings &settings, std::uint32_t requestedUpdates);
    bool StartUpdateCheck(UpdateCheckMode mode);
    void HandleUpdateCheckResult(const UpdateCheckResult &result);

    // Message handlers
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnDpiChanged(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnMove(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnNcHitTest(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnLButtonDown(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnLButtonUp(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnCaptureChanged(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnSetCursor(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnSearchEditColor(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL&);
    LRESULT OnPopupActivation(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnSearchChanged(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnHistoryListSelectionChanged(WORD, WORD id, HWND, BOOL& handled);
    LRESULT OnClearHistoryButton(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnSettingsButton(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnAboutButton(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnPreviewToggleButton(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnExitButton(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnTrayShowCommand(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnTraySettingsCommand(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnTrayClearCommand(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnTrayIgnoreCommand(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnTrayExitCommand(WORD, WORD, HWND, BOOL& handled);
    LRESULT OnTimer(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnHotKey(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnKeyUp(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnImeStart(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnImeEnd(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL&);
    LRESULT OnUiUpdate(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnClipboardUpdate(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnPreviewWorkerResult(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnStorageWorkerResult(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnUpdateCheckerResult(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnDestroy(UINT, WPARAM, LPARAM, BOOL&);

    // Callback handlers for KeyboardHandler
    static void OnPreviewCallback(void* context, sqlite3_int64 itemId, bool keyboard);
    static void OnPasteCallback(void* context, int index);
    static void OnScheduleSearchCallback(void* context);
    static void OnTogglePinCallback(void* context);
    static void OnDeleteItemCallback(void* context);
    static void OnTogglePreviewCallback(void* context);
    static void OnClearHistoryCallback(void* context, bool all);
    static void OnOpenSettingsCallback(void* context);
    static void OnExitCallback(void* context);
    static void OnHideWindowCallback(void* context);

    StorageWorker &m_storage;
    AppSettings m_settings;
    bool m_suppressClearAlert = false;
    StorageWorker::IgnoreLists m_ignoredLists;
    ClipboardMonitor m_clipboard;
    PreviewWorker &m_previewWorker;

    // Components
    PasteController m_pasteController;
    HistoryRenderer m_historyRenderer;
    KeyboardHandler m_keyboardHandler;
    UpdateChecker m_updateChecker;
    std::unique_ptr<SettingsWindow> m_settingsWindow;

    // UI controls
    CEdit m_search;
    CListBox m_historyList;
    CListBox m_pinsList;
    CButton m_previewToggle;
    CToolTipCtrl m_tooltips;
    CButton m_footerClear;
    CButton m_footerSettings;
    CButton m_footerAbout;
    CButton m_footerExit;

    // Window procedures
    WNDPROC m_originalSearchProc = nullptr;
    WNDPROC m_originalHistoryListProc = nullptr;
    WNDPROC m_originalPinsProc = nullptr;

    // History data
    std::vector<ClipboardItem> m_items;
    std::wstring m_searchQuery;
    sqlite3_int64 m_previewCandidateId = 0;
    sqlite3_int64 m_previewItemId = 0;
    bool m_previewSuppressed = false;
    bool m_popupVisible = false;
    bool m_searchClearPressed = false;
    PopupPosition m_activePopupPosition = PopupPosition::Cursor;
    bool m_loadingList = false;
    std::uint32_t m_pendingUpdates = 0;
    bool m_updateMessagePosted = false;
    std::uint64_t m_historyGeneration = 0;
    struct DeferredHistoryResult {
        std::uint64_t generation = 0;
        std::wstring query;
        std::vector<ClipboardItem> items;
    };
    std::optional<DeferredHistoryResult> m_deferredHistoryResult;

    // Layout
    SearchHeaderLayout::Geometry m_searchHeader{};
    int m_pinSeparatorY = -1;
    int m_footerSeparatorY = -1;

    // Preview state
    std::wstring m_previewTip;
    std::uint64_t m_previewGeneration = 0;
    bool m_pasteInProgress = false;

    // Tray icon
    NOTIFYICONDATAW m_notifyIcon{};
    HICON m_trayIcon = nullptr;
    bool m_trayIconAdded = false;

    // State flags
    bool m_modalShowing = false;
    bool m_exiting = false;
    bool m_trayMenuShowing = false;
    bool m_isolated = false;
};
