#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlwin.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "ClipboardMonitor.h"
#include "Database.h"
#include "HistoryRenderer.h"
#include "KeyboardHandler.h"
#include "PreviewWindow.h"
#include "Settings.h"
#include "resource.h"
#include "SettingsWindow.h"

class SettingsWindow;

// Main application window - coordinates all components
class MainWindow : public CDialogImpl<MainWindow> {
    friend struct MainWindowTests;
public:
    enum { IDD = IDD_HISTORY };

    enum class PreviewSource {
        None,
        Mouse,
        Keyboard,
    };

    explicit MainWindow(Database& database, bool isolated = false);
    ~MainWindow();

    BEGIN_MSG_MAP(MainWindow)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_EXITSIZEMOVE, OnExitSizeMove)
        MESSAGE_HANDLER(WM_ENTERSIZEMOVE, OnEnterSizeMove)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnEditColor)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnEditColor)
        MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)
        MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_CLIPBOARDUPDATE, OnClipboardUpdate)
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
        MESSAGE_HANDLER(AppConstants::kSettingsChangedMessage, OnSettingsChanged)
    END_MSG_MAP()

    bool AddTrayIcon();
    void ShowMainWindow();

private:
    // Window procedure callbacks for subclassed controls
    static LRESULT CALLBACK SearchWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK HistoryListWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK MenuControlProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                           UINT_PTR, DWORD_PTR data);

    // Control management
    bool BindControls();
    void LayoutHistoryControls();
    void RedrawHistoryLists();
    void RedrawFooterButtons();
    void RestoreControlSubclass(HWND control, WNDPROC original);
    void RestoreControlSubclasses();
    std::array<HWND, 4> FooterButtons() const;

    // Window positioning
    void PositionPopup();
    void PositionOnMonitor(HMONITOR monitor, bool center);
    void ConstrainPopupToWorkArea();
    void PositionPreviewWindow();
    HMONITOR SelectedMonitor() const;
    int PopupWidth() const;
    int PopupHeight() const;

    // History management
    void RefreshHistory(std::wstring_view query);
    void ApplyHistoryVisibility();
    void SetHistorySearchVisible(bool visible);

    // Preview window
    void SchedulePreviewForItem(sqlite3_int64 item_id, PreviewSource source);
    void ShowPreviewForItem(sqlite3_int64 item_id);
    void ShowPreviewForCandidate();
    void ShowPreviewForSelection();
    void HidePreview();
    void TogglePreview();

    // Actions
    void PasteItem(int index);
    void PasteSelectedItem();
    void ToggleSelectedPin();
    void DeleteSelectedItem();
    void ClearHistory(bool all = false);
    void OpenAbout();
    void OpenSettings();
    void ExitApplication();

    // Utilities
    void UpdateFooterControls();
    void ScheduleSearch();
    void ScheduleSearchFromCurrentEdit();
    void SaveWindowGeometry(bool resized = false);
    void HideMainWindow();
    bool HasHistoryItems() const;
    int SelectedHistoryIndex() const;
    std::wstring NextPinKey() const;
    bool IsOurWindow(HWND window) const;

    // Tray icon
    void UpdateTrayTooltip();
    void UpdateTrayIcon();
    void ShowTrayMenu();
    void RemoveTrayIcon();

    // Settings
    void ApplySettings();

    // Message handlers
    LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnSize(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnMove(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnExitSizeMove(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnEnterSizeMove(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnGetMinMaxInfo(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnNcHitTest(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnPaint(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEraseBackground(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnEditColor(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL& handled);
    LRESULT OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL&);
    LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnCommand(UINT, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnTimer(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnClipboardUpdate(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT OnHotKey(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnKeyUp(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL& handled);
    LRESULT OnImeStart(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnImeEnd(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT OnTrayIcon(UINT, WPARAM wParam, LPARAM lParam, BOOL&);
    LRESULT OnSettingsChanged(UINT, WPARAM, LPARAM, BOOL& handled);
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

    Database& m_database;
    AppSettings m_settings;

    // Components
    ClipboardMonitor m_clipboardMonitor;
    HistoryRenderer m_historyRenderer;
    KeyboardHandler m_keyboardHandler;
    PreviewWindow m_previewWindow;
    std::unique_ptr<SettingsWindow> m_settingsWindow;

    // UI controls
    HWND m_search = nullptr;
    HWND m_historyList = nullptr;
    HWND m_pinsList = nullptr;
    HWND m_searchClear = nullptr;
    HWND m_previewToggle = nullptr;
    HWND m_tooltips = nullptr;
    HWND m_footerClear = nullptr;
    HWND m_footerSettings = nullptr;
    HWND m_footerAbout = nullptr;
    HWND m_footerExit = nullptr;

    // Window procedures
    WNDPROC m_originalSearchProc = nullptr;
    WNDPROC m_originalHistoryListProc = nullptr;
    WNDPROC m_originalPinsProc = nullptr;

    // History data
    std::vector<ClipboardItem> m_items;
    std::wstring m_searchQuery;

    // Layout
    RECT m_searchRect{};
    RECT m_titleRect{};
    int m_searchHeight = 0;
    int m_pinSeparatorY = -1;
    int m_footerSeparatorY = -1;

    // Preview state
    sqlite3_int64 m_previewCandidateId = 0;
    sqlite3_int64 m_previewItemId = 0;
    PreviewSource m_previewSource = PreviewSource::None;
    bool m_previewSuppressed = false;
    std::wstring m_previewTip;

    // Tray icon
    NOTIFYICONDATAW m_notifyIcon{};
    bool m_trayIconAdded = false;

    // State flags
    bool m_popupVisible = false;
    bool m_loadingList = false;
    bool m_modalShowing = false;
    bool m_exiting = false;
    bool m_trayMenuShowing = false;
    bool m_isolated = false;
    bool m_inSizeMove = false;
};
