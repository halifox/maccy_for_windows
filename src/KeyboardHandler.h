#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <array>
#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <cstdint>
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

    // Hotkey management
    bool RegisterGlobalHotKey(UINT hotkeyId);
    void UnregisterGlobalHotKey(UINT hotkeyId);
    bool IsHotkeyRegistered() const { return m_hotkeyRegistered; }

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
    bool IsComposing(HWND window) const;
    void SetImeComposing(bool composing) { m_imeComposing = composing; }

    // Helper for list operations
    int ItemIndexAtRow(CListBox list, int row,
                       const std::vector<ClipboardItem>& items) const;
    int HistoryItemAtPoint(CListBox window, POINT point,
                           CListBox historyList, CListBox pinsList,
                           const std::vector<ClipboardItem>& items) const;
    void InvalidateHistoryItem(int index, const std::vector<ClipboardItem>& items,
                              CListBox historyList, CListBox pinsList);

    // Callbacks
    using PreviewCallback = void (*)(void* context, sqlite3_int64 itemId, bool keyboard);
    using PasteCallback = void (*)(void* context, int index);
    using SearchCallback = void (*)(void* context);
    using TogglePinCallback = void (*)(void* context);
    using DeleteItemCallback = void (*)(void* context);
    using TogglePreviewCallback = void (*)(void* context);
    using ClearHistoryCallback = void (*)(void* context, bool all);
    using OpenSettingsCallback = void (*)(void* context);
    using ExitCallback = void (*)(void* context);
    using HideWindowCallback = void (*)(void* context);

    void SetCallbacks(void* context,
                     PreviewCallback previewCb,
                     PasteCallback pasteCb,
                     SearchCallback searchCb,
                     TogglePinCallback togglePinCb,
                     DeleteItemCallback deleteItemCb,
                     TogglePreviewCallback togglePreviewCb,
                     ClearHistoryCallback clearHistoryCb,
                     OpenSettingsCallback openSettingsCb,
                     ExitCallback exitCb,
                     HideWindowCallback hideWindowCb);

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
    bool m_keyboardNavigating = false;
    bool m_imeComposing = false;

    POINT m_keyboardPointer{};

    int m_activeItemIndex = -1;
    sqlite3_int64 m_activeItemId = 0;
    int m_hoveredItemIndex = -1;
    sqlite3_int64 m_hoveredItemId = 0;
    int m_activeFooter = -1;

    CListBox m_hoverList;

    // Callbacks
    void* m_callbackContext = nullptr;
    PreviewCallback m_previewCallback = nullptr;
    PasteCallback m_pasteCallback = nullptr;
    SearchCallback m_searchCallback = nullptr;
    TogglePinCallback m_togglePinCallback = nullptr;
    DeleteItemCallback m_deleteItemCallback = nullptr;
    TogglePreviewCallback m_togglePreviewCallback = nullptr;
    ClearHistoryCallback m_clearHistoryCallback = nullptr;
    OpenSettingsCallback m_openSettingsCallback = nullptr;
    ExitCallback m_exitCallback = nullptr;
    HideWindowCallback m_hideWindowCallback = nullptr;
};
