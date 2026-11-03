#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <array>
#include <atomic>
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "ClipboardData.h"
#include "Settings.h"

// Keyboard navigation and input handling component
class KeyboardHandler {
public:
    explicit KeyboardHandler(AppSettings& settings);
    ~KeyboardHandler();

    KeyboardHandler(const KeyboardHandler&) = delete;
    KeyboardHandler& operator=(const KeyboardHandler&) = delete;

    // Lifecycle
    bool Initialize(CWindow owner, CEdit search, CListBox historyList, CListBox pinsList,
                    const std::array<CButton, AppConstants::UI::kFooterButtonCount>& footerButtons);
    void Shutdown();

    // Low-level keyboard hook for the open-popup chord (not RegisterHotKey).
    bool InstallOpenHotkeyHook(UINT hotkeyId);
    void UninstallOpenHotkeyHook(UINT hotkeyId);
    bool IsHotkeyRegistered() const { return m_hotkeyRegistered; }
    static void SetHotkeyCaptureActive(bool active) noexcept;
    static bool IsHotkeyCaptureActive() noexcept;

    // Input handling
    bool HandlePopupKey(WPARAM key, const CEdit& search, const std::vector<ClipboardItem>& items);
    bool HandlePopupShortcut(WPARAM key, const std::vector<ClipboardItem>& items);
    bool OnChar(WPARAM character);

    // Navigation
    void SetActiveHistoryItem(int index, const std::vector<ClipboardItem>& items,
                              CListBox historyList, CListBox pinsList,
                              bool scrollIntoView = true);
    void NavigateHistoryFromSearch(bool forward, const std::vector<ClipboardItem>& items,
                                   CListBox historyList, CListBox pinsList, bool showFooter);
    int GetActiveItemIndex() const { return m_activeItemIndex; }
    sqlite3_int64 GetActiveItemId() const { return m_activeItemId; }
    int GetActiveFooter() const { return m_activeFooter; }

    // Mouse interaction
    void OnHistoryMouseMove(CListBox window, POINT point,
                            const std::vector<ClipboardItem>& items,
                            CListBox historyList, CListBox pinsList,
                            bool popupVisible);
    void OnHistoryMouseLeave();
    void UpdateHistoryHoverFromCursor(const std::vector<ClipboardItem>& items,
                                     CListBox historyList, CListBox pinsList,
                                     bool popupVisible);
    void ClearHistoryHover();

    // Selection state
    int GetHoveredItemIndex() const { return m_hoveredItemIndex; }
    sqlite3_int64 GetHoveredItemId() const { return m_hoveredItemId; }
    bool IsKeyboardNavigating() const { return m_keyboardNavigating; }

    // Footer navigation
    void SetActiveFooter(int index, const std::vector<ClipboardItem>& items);

    // Utilities
    bool MouseCanSelect();
    void TypeToSearch(WPARAM character, CEdit& search);
    void FocusSearchOrPopup(CEdit& search, CWindow mainWindow, bool searchVisible);

    // IME
    bool IsComposing(CWindow window) const;
    void SetImeComposing(bool composing) { m_imeComposing = composing; }

    // Helper for list operations
    int ItemIndexAtRow(CListBox list, int row,
                       const std::vector<ClipboardItem>& items) const;
    int HistoryItemAtPoint(CListBox window, POINT point,
                           CListBox historyList, CListBox pinsList,
                           const std::vector<ClipboardItem>& items) const;
    void InvalidateHistoryItem(int index, const std::vector<ClipboardItem>& items,
                              CListBox historyList, CListBox pinsList);

    // Owner actions (std::function — L-18; no void* trampolines).
    using PreviewCallback = std::function<void(sqlite3_int64 itemId, bool keyboard)>;
    using PasteCallback = std::function<void(int index)>;
    using SearchCallback = std::function<void()>;
    using TogglePinCallback = std::function<void()>;
    using DeleteItemCallback = std::function<void()>;
    using TogglePreviewCallback = std::function<void()>;
    using ClearHistoryCallback = std::function<void(bool all)>;
    using OpenSettingsCallback = std::function<void()>;
    using ExitCallback = std::function<void()>;
    using HideWindowCallback = std::function<void()>;

    struct Callbacks {
        PreviewCallback preview;
        PasteCallback paste;
        SearchCallback search;
        TogglePinCallback toggle_pin;
        DeleteItemCallback delete_item;
        TogglePreviewCallback toggle_preview;
        ClearHistoryCallback clear_history;
        OpenSettingsCallback open_settings;
        ExitCallback exit;
        HideWindowCallback hide_window;
    };

    void SetCallbacks(Callbacks callbacks);

private:
    // Helper methods
    CListBox ListForItem(int index, const std::vector<ClipboardItem>& items,
                         CListBox historyList, CListBox pinsList) const;
    int RowForItem(int index, const std::vector<ClipboardItem>& items,
                   CListBox historyList, CListBox pinsList) const;
    void SetListSelection(CListBox list, int row) const;
    void InvalidateFooterButtons() const;
    void BeginHistoryMouseTracking();

    AppSettings& m_settings;
    CWindow m_owner;
    CEdit m_search;
    CListBox m_historyList;
    CListBox m_pinsList;
    std::array<CButton, AppConstants::UI::kFooterButtonCount> m_footerButtons{};

    bool m_hotkeyRegistered = false;
    HHOOK m_keyboardHook = nullptr;
    bool m_suppressingOpenKey = false;
    UINT m_suppressingCaptureKey = 0;
    HWND m_captureTargetWindow = nullptr;
    UINT m_capturedKey = 0;
    UINT m_capturedModifiers = 0;
    bool m_capturedKeyReleased = false;
    UINT m_pendingWinKey = 0;
    DWORD m_pendingWinScan = 0;
    DWORD m_pendingWinFlags = 0;
    bool m_winKeyPassedThrough = false;
    bool m_suppressingWinKey = false;
    // Win+key open: wait for Win-up before posting so show is not fought by the shell.
    bool m_pendingOpenPopup = false;
    HWND m_pendingOpenTarget = nullptr;

    static LRESULT CALLBACK LowLevelKeyboardProc(int code, WPARAM message, LPARAM data);
    static KeyboardHandler* s_hookOwner;
    static std::atomic<bool> s_hotkeyCaptureActive;
    bool m_keyboardNavigating = false;
    bool m_imeComposing = false;

    POINT m_keyboardPointer{};

    int m_activeItemIndex = -1;
    sqlite3_int64 m_activeItemId = 0;
    int m_hoveredItemIndex = -1;
    sqlite3_int64 m_hoveredItemId = 0;
    int m_activeFooter = -1;

    CListBox m_hoverList;

    Callbacks m_callbacks;
};
