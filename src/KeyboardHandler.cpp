#include "KeyboardHandler.h"

#include <array>
#include <imm.h>
#include <windowsx.h>

namespace {

constexpr UINT_PTR kSearchTimerId = 1;
constexpr UINT_PTR kPreviewTimerId = 2;

} // namespace

KeyboardHandler::KeyboardHandler(AppSettings& settings)
    : m_settings(settings) {}

KeyboardHandler::~KeyboardHandler() {
    Shutdown();
}

bool KeyboardHandler::Initialize(HWND owner, HWND search, HWND historyList, HWND pinsList) {
    m_owner = owner;
    m_search = search;
    m_historyList = historyList;
    m_pinsList = pinsList;
    return true;
}

void KeyboardHandler::Shutdown() {
    if (m_hotkeyRegistered && m_owner != nullptr) {
        UnregisterHotKey(m_owner, 1006); // kHotkeyId
        m_hotkeyRegistered = false;
    }
}

bool KeyboardHandler::RegisterGlobalHotKey(UINT hotkeyId) {
    m_hotkeyRegistered = RegisterHotKey(
        m_owner,
        hotkeyId,
        m_settings.open_hotkey.modifiers | MOD_NOREPEAT,
        m_settings.open_hotkey.virtual_key
    ) == TRUE;
    return m_hotkeyRegistered;
}

void KeyboardHandler::UnregisterGlobalHotKey(UINT hotkeyId) {
    if (m_hotkeyRegistered && m_owner != nullptr) {
        UnregisterHotKey(m_owner, hotkeyId);
        m_hotkeyRegistered = false;
    }
}

bool KeyboardHandler::IsComposing(HWND window) const {
    HIMC context = ImmGetContext(window);
    const bool composing = context && ImmGetCompositionStringW(context, GCS_COMPSTR, nullptr, 0) > 0;
    if (context) ImmReleaseContext(window, context);
    return m_imeComposing || composing;
}

bool KeyboardHandler::MouseCanSelect() {
    POINT position{};
    GetCursorPos(&position);
    if (m_keyboardNavigating && position.x == m_keyboardPointer.x && position.y == m_keyboardPointer.y) {
        return false;
    }
    m_keyboardNavigating = false;
    return true;
}

void KeyboardHandler::TypeToSearch(WPARAM character, HWND search) {
    if (!m_settings.show_search) return;
    ::SetFocus(search);
    const wchar_t text[] = {static_cast<wchar_t>(character), 0};
    SendMessageW(search, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(text));
}

void KeyboardHandler::FocusSearchOrPopup(HWND search, HWND mainWindow, bool searchVisible) {
    ::SetFocus(searchVisible ? search : mainWindow);
}

void KeyboardHandler::SelectFooter(int index, int oldActiveIndex,
                                   HWND historyList, HWND pinsList,
                                   const std::array<HWND, 4>& footerButtons) {
    if (m_activeFooter == index) return;
    m_activeFooter = index;
    InvalidateHistoryItem(oldActiveIndex, {}, historyList, pinsList);
    for (HWND button : footerButtons) {
        ::InvalidateRect(button, nullptr, FALSE);
    }
}

int KeyboardHandler::ItemIndex(HWND list, int row, const std::vector<ClipboardItem>& items) const {
    if (row < 0) return -1;
    const LRESULT value = SendMessageW(list, LB_GETITEMDATA, row, 0);
    return value == LB_ERR ? -1 : static_cast<int>(value);
}

HWND KeyboardHandler::ListForItem(int index, const std::vector<ClipboardItem>& items,
                                  HWND historyList, HWND pinsList) const {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return nullptr;
    return items[index].pinned ? pinsList : historyList;
}

int KeyboardHandler::RowForItem(int index, const std::vector<ClipboardItem>& items,
                               HWND historyList, HWND pinsList) const {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return -1;
    HWND list = ListForItem(index, items, historyList, pinsList);
    if (list == nullptr) return -1;
    const int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
    for (int row = 0; row < count; ++row) {
        if (ItemIndex(list, row, items) == index) return row;
    }
    return -1;
}

void KeyboardHandler::InvalidateHistoryItem(int index, const std::vector<ClipboardItem>& items,
                                           HWND historyList, HWND pinsList) {
    const int row = RowForItem(index, items, historyList, pinsList);
    if (row < 0) return;
    RECT rect{};
    HWND list = ListForItem(index, items, historyList, pinsList);
    if (list && SendMessageW(list, LB_GETITEMRECT, row, reinterpret_cast<LPARAM>(&rect)) != LB_ERR) {
        ::InvalidateRect(list, &rect, FALSE);
    }
}

void KeyboardHandler::SetActiveHistoryItem(int index, const std::vector<ClipboardItem>& items,
                                          HWND historyList, HWND pinsList) {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return;
    const int previous = m_activeItemIndex;
    m_activeItemIndex = index;
    m_activeItemId = items[index].id;
    m_activeFooter = -1;

    SendMessageW(historyList, LB_SETCURSEL, -1, 0);
    SendMessageW(pinsList, LB_SETCURSEL, -1, 0);
    HWND list = ListForItem(index, items, historyList, pinsList);
    if (list) {
        SendMessageW(list, LB_SETCURSEL, RowForItem(index, items, historyList, pinsList), 0);
    }

    InvalidateHistoryItem(previous, items, historyList, pinsList);
    InvalidateHistoryItem(index, items, historyList, pinsList);
}

int KeyboardHandler::HistoryItemAtPoint(HWND window, POINT point,
                                       HWND historyList, HWND pinsList,
                                       const std::vector<ClipboardItem>& items) const {
    if (window != historyList && window != pinsList) return -1;
    const LRESULT hit = SendMessageW(window, LB_ITEMFROMPOINT, 0, MAKELPARAM(point.x, point.y));
    return HIWORD(hit) ? -1 : ItemIndex(window, LOWORD(hit), items);
}

void KeyboardHandler::BeginHistoryMouseTracking() {
    TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, m_hoverList, 0};
    ::TrackMouseEvent(&tracking);
}

void KeyboardHandler::OnHistoryMouseMove(HWND window, POINT point,
                                        const std::vector<ClipboardItem>& items,
                                        HWND historyList, HWND pinsList,
                                        bool popupVisible, bool showSearch) {
    if (!MouseCanSelect()) return;
    if (!popupVisible || (window != historyList && window != pinsList)) {
        return;
    }

    m_hoverList = window;
    BeginHistoryMouseTracking();

    const int index = HistoryItemAtPoint(window, point, historyList, pinsList, items);
    if (index < 0) {
        ClearHistoryHover();
        return;
    }

    const sqlite3_int64 item_id = items[static_cast<size_t>(index)].id;
    if (index == m_hoveredItemIndex && item_id == m_hoveredItemId) {
        SetActiveHistoryItem(index, items, historyList, pinsList);
        if (m_previewCallback && m_callbackContext) {
            m_previewCallback(m_callbackContext, item_id, false);
        }
        return;
    }

    const int previous_index = m_hoveredItemIndex;
    m_hoveredItemIndex = index;
    m_hoveredItemId = item_id;
    SetActiveHistoryItem(index, items, historyList, pinsList);

    InvalidateHistoryItem(previous_index, items, historyList, pinsList);
    InvalidateHistoryItem(index, items, historyList, pinsList);

    if (m_previewCallback && m_callbackContext) {
        m_previewCallback(m_callbackContext, item_id, false);
    }
}

void KeyboardHandler::OnHistoryMouseLeave() {
    ClearHistoryHover();
}

void KeyboardHandler::UpdateHistoryHoverFromCursor(const std::vector<ClipboardItem>& items,
                                                   HWND historyList, HWND pinsList,
                                                   bool popupVisible) {
    if (!popupVisible) return;
    POINT cursor{};
    if (!GetCursorPos(&cursor)) return;
    for (HWND list : {historyList, pinsList}) {
        if (!::IsWindowVisible(list)) continue;
        POINT point = cursor;
        ::ScreenToClient(list, &point);
        RECT rect{};
        ::GetClientRect(list, &rect);
        if (::PtInRect(&rect, point)) {
            OnHistoryMouseMove(list, point, items, historyList, pinsList, popupVisible, true);
            return;
        }
    }
    ClearHistoryHover();
}

void KeyboardHandler::ClearHistoryHover() {
    m_hoveredItemIndex = -1;
    m_hoveredItemId = 0;
    if (m_owner) {
        KillTimer(m_owner, kPreviewTimerId);
    }
}

void KeyboardHandler::NavigateHistoryFromSearch(bool forward,
                                               const std::vector<ClipboardItem>& items,
                                               HWND historyList, HWND pinsList,
                                               bool showFooter) {
    m_keyboardNavigating = true;
    GetCursorPos(&m_keyboardPointer);

    const int count = static_cast<int>(items.size());
    const int total = count + (showFooter ? 4 : 0);
    if (!total) return;

    int current = m_activeFooter >= 0 ? count + m_activeFooter : m_activeItemIndex;
    const int target = std::clamp(current + (forward ? 1 : -1), 0, total - 1);

    if (target >= count) {
        // Navigate to footer
        m_hoveredItemIndex = -1;
        m_hoveredItemId = 0;
        m_activeFooter = target - count;
        m_activeItemIndex = -1;
    } else {
        SetActiveHistoryItem(target, items, historyList, pinsList);
        if (m_previewCallback && m_callbackContext) {
            m_previewCallback(m_callbackContext, m_activeItemId, true);
        }
    }
}

bool KeyboardHandler::HandlePopupKey(WPARAM key, HWND search, const std::vector<ClipboardItem>& items) {
    if (m_searchCallback && search &&
        (key == VK_RETURN || key == VK_UP || key == VK_DOWN || key == VK_PRIOR || key == VK_NEXT ||
         (GetKeyState(VK_CONTROL) & 0x8000) || (GetKeyState(VK_MENU) & 0x8000))) {
        m_searchCallback(m_callbackContext);
    }

    if (HandlePopupShortcut(key, items)) return true;

    const bool ctrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
    if (key == VK_ESCAPE) {
        if (m_hideWindowCallback && m_callbackContext) {
            m_hideWindowCallback(m_callbackContext);
        }
        return true;
    }
    if (key == VK_RETURN) {
        if (m_activeFooter >= 0) {
            // Handle footer button
        } else if (m_pasteCallback && m_callbackContext) {
            m_pasteCallback(m_callbackContext, m_activeItemIndex);
        }
        return true;
    }
    if (key == VK_UP || key == VK_DOWN || key == VK_TAB) {
        return true;
    }
    if ((ctrl && (key == VK_HOME || key == VK_END)) || key == VK_PRIOR || key == VK_NEXT) {
        m_keyboardNavigating = true;
        GetCursorPos(&m_keyboardPointer);
        if (!items.empty()) {
            const int target = (key == VK_HOME || key == VK_PRIOR) ? 0 : static_cast<int>(items.size()) - 1;
            SetActiveHistoryItem(target, items, m_historyList, m_pinsList);
        }
        return true;
    }
    if (ctrl && key == 'F' && m_settings.show_search) {
        return true;
    }
    if (ctrl && key == 'U') {
        return true;
    }
    return false;
}

bool KeyboardHandler::HandlePopupShortcut(WPARAM key, const std::vector<ClipboardItem>& items) {
    if (IsHotKeyPressed(m_settings.pin_hotkey, key)) {
        if (m_togglePinCallback && m_callbackContext) {
            m_togglePinCallback(m_callbackContext);
        }
        return true;
    }
    if (IsHotKeyPressed(m_settings.delete_hotkey, key)) {
        if (m_deleteItemCallback && m_callbackContext) {
            m_deleteItemCallback(m_callbackContext);
        }
        return true;
    }
    if (IsHotKeyPressed(m_settings.preview_hotkey, key)) {
        if (m_togglePreviewCallback && m_callbackContext) {
            m_togglePreviewCallback(m_callbackContext);
        }
        return true;
    }

    const bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (GetKeyState(VK_MENU) & 0x8000) != 0;
    const bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;

    if (ctrl && alt && key == VK_BACK) {
        if (m_clearHistoryCallback && m_callbackContext) {
            m_clearHistoryCallback(m_callbackContext, shift);
        }
        return true;
    }
    if (ctrl && !alt && key == VK_OEM_COMMA) {
        if (m_openSettingsCallback && m_callbackContext) {
            m_openSettingsCallback(m_callbackContext);
        }
        return true;
    }
    if (ctrl && !alt && key == 'Q') {
        if (m_exitCallback && m_callbackContext) {
            m_exitCallback(m_callbackContext);
        }
        return true;
    }

    // Keep native Edit shortcuts and global popup navigation available.
    if (ctrl && !alt && std::wstring_view(L"ACFUVXYZ").find(static_cast<wchar_t>(key)) != std::wstring_view::npos) {
        return false;
    }

    if ((ctrl != alt) && !(GetKeyState(VK_LWIN) & 0x8000) && !(GetKeyState(VK_RWIN) & 0x8000)) {
        int number = 0;
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            const bool matches = item.pinned
                ? item.pin.size() == 1 && towupper(item.pin[0]) == key
                : (++number <= 9 && key == static_cast<WPARAM>('0' + number));
            if (matches) {
                SetActiveHistoryItem(static_cast<int>(i), items, m_historyList, m_pinsList);
                if (m_pasteCallback && m_callbackContext) {
                    m_pasteCallback(m_callbackContext, static_cast<int>(i));
                }
                return true;
            }
        }
    }
    return false;
}

void KeyboardHandler::OnKeyUp() {
    // Can be used to update UI state
}

bool KeyboardHandler::OnChar(WPARAM character) {
    return m_settings.show_search && character >= 0x20 && character != 0x7f;
}

void KeyboardHandler::SetCallbacks(void* context,
                                  PreviewCallback previewCb,
                                  PasteCallback pasteCb,
                                  SearchCallback searchCb,
                                  TogglePinCallback togglePinCb,
                                  DeleteItemCallback deleteItemCb,
                                  TogglePreviewCallback togglePreviewCb,
                                  ClearHistoryCallback clearHistoryCb,
                                  OpenSettingsCallback openSettingsCb,
                                  ExitCallback exitCb,
                                  HideWindowCallback hideWindowCb) {
    m_callbackContext = context;
    m_previewCallback = previewCb;
    m_pasteCallback = pasteCb;
    m_searchCallback = searchCb;
    m_togglePinCallback = togglePinCb;
    m_deleteItemCallback = deleteItemCb;
    m_togglePreviewCallback = togglePreviewCb;
    m_clearHistoryCallback = clearHistoryCb;
    m_openSettingsCallback = openSettingsCb;
    m_exitCallback = exitCb;
    m_hideWindowCallback = hideWindowCb;
}
