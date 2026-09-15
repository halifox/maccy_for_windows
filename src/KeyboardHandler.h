#pragma once

#include "PlatformConfig.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "Database.h"
#include "Settings.h"

class HistoryView;

// Keyboard navigation and input handling component
class KeyboardHandler {
public:
    explicit KeyboardHandler(AppSettings& settings);
    ~KeyboardHandler();

    KeyboardHandler(const KeyboardHandler&) = delete;
    KeyboardHandler& operator=(const KeyboardHandler&) = delete;

    // Lifecycle
    bool Initialize(HWND owner, HWND search, HWND historyList, HWND pinsList,
                    const std::array<HWND, 4>& footerButtons);
    void SetHistoryView(HistoryView& view) noexcept { m_historyView = &view; }
    void Shutdown();

    // Hotkey management
    bool RegisterGlobalHotKey(UINT hotkeyId);
    void UnregisterGlobalHotKey(UINT hotkeyId);
    bool IsHotkeyRegistered() const { return m_hotkeyRegistered; }

    // Input handling
    bool HandlePopupKey(WPARAM key, HWND search, const std::vector<ClipboardItem>& items);
    bool HandlePopupShortcut(WPARAM key, const std::vector<ClipboardItem>& items);
    bool OnChar(WPARAM character);

    // Navigation
    void SetActiveHistoryItem(int index, const std::vector<ClipboardItem>& items,
                              HWND historyList, HWND pinsList,
                              bool scrollIntoView = true);
    void NavigateHistoryFromSearch(bool forward, const std::vector<ClipboardItem>& items,
                                   HWND historyList, HWND pinsList, bool showFooter);
    int GetActiveItemIndex() const { return m_activeItemIndex; }
    sqlite3_int64 GetActiveItemId() const { return m_activeItemId; }
    int GetActiveFooter() const { return m_activeFooter; }

    // Mouse interaction
    void OnHistoryMouseMove(HWND window, POINT point,
                            const std::vector<ClipboardItem>& items,
                            HWND historyList, HWND pinsList,
                            bool popupVisible);
    void OnHistoryMouseLeave();
    void UpdateHistoryHoverFromCursor(const std::vector<ClipboardItem>& items,
                                     HWND historyList, HWND pinsList,
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
    void TypeToSearch(WPARAM character, HWND search);
    void FocusSearchOrPopup(HWND search, HWND mainWindow, bool searchVisible);

    // IME
    bool IsComposing(HWND window) const;
    void SetImeComposing(bool composing) { m_imeComposing = composing; }

    // Helper for list operations
    void InvalidateHistoryItem(int index, const std::vector<ClipboardItem>& items,
                              HWND historyList, HWND pinsList);

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
    HWND ListForItem(int index, const std::vector<ClipboardItem>& items,
                    HWND historyList, HWND pinsList) const;
    int RowForItem(int index, const std::vector<ClipboardItem>& items,
                  HWND historyList, HWND pinsList) const;
    void SetListSelection(HWND list, int row) const;
    void InvalidateFooterButtons() const;
    void BeginHistoryMouseTracking();

    AppSettings& m_settings;
    HistoryView* m_historyView = nullptr;
    HWND m_owner = nullptr;
    HWND m_search = nullptr;
    HWND m_historyList = nullptr;
    HWND m_pinsList = nullptr;
    std::array<HWND, 4> m_footerButtons{};

    bool m_hotkeyRegistered = false;
    bool m_keyboardNavigating = false;
    bool m_imeComposing = false;

    POINT m_keyboardPointer{};

    int m_activeItemIndex = -1;
    sqlite3_int64 m_activeItemId = 0;
    int m_hoveredItemIndex = -1;
    sqlite3_int64 m_hoveredItemId = 0;
    int m_activeFooter = -1;

    HWND m_hoverList = nullptr;

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
