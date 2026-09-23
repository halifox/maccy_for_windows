#include "KeyboardHandler.h"
#include "Constants.h"

#include <algorithm>
#include <array>
#include <imm.h>
#include <windowsx.h>

KeyboardHandler::KeyboardHandler(AppSettings& settings)
    : m_settings(settings) {}

KeyboardHandler::~KeyboardHandler() {
    Shutdown();
}

bool KeyboardHandler::Initialize(CWindow owner, CEdit search, CListBox historyList,
                                 CListBox pinsList,
                                 const std::array<CButton, AppConstants::UI::kFooterButtonCount>& footerButtons) {
    m_owner = owner;
    m_search = search;
    m_historyList = historyList;
    m_pinsList = pinsList;
    m_footerButtons = footerButtons;
    return true;
}

void KeyboardHandler::Shutdown() {
    if (m_hotkeyRegistered && m_owner.m_hWnd != nullptr) {
        UnregisterHotKey(m_owner.m_hWnd, AppConstants::HotKey::kOpenPopup);
        m_hotkeyRegistered = false;
    }
}

bool KeyboardHandler::RegisterGlobalHotKey(UINT hotkeyId) {
    m_hotkeyRegistered = RegisterHotKey(
        m_owner.m_hWnd,
        hotkeyId,
        m_settings.open_hotkey.modifiers | MOD_NOREPEAT,
        m_settings.open_hotkey.virtual_key
    ) == TRUE;
    return m_hotkeyRegistered;
}

void KeyboardHandler::UnregisterGlobalHotKey(UINT hotkeyId) {
    if (m_hotkeyRegistered && m_owner.m_hWnd != nullptr) {
        UnregisterHotKey(m_owner.m_hWnd, hotkeyId);
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

void KeyboardHandler::TypeToSearch(WPARAM character, CEdit& search) {
    if (!m_settings.show_search) return;
    search.SetFocus();
    const wchar_t text[] = {static_cast<wchar_t>(character), 0};
    search.ReplaceSel(text);
}

void KeyboardHandler::FocusSearchOrPopup(CEdit& search, CWindow mainWindow, bool searchVisible) {
    if (searchVisible) {
        search.SetFocus();
    } else {
        mainWindow.SetFocus();
    }
}

void KeyboardHandler::SetActiveFooter(int index, const std::vector<ClipboardItem>& items) {
    if (index < 0 || index >= static_cast<int>(m_footerButtons.size()) ||
        (m_activeFooter == index && m_activeItemIndex < 0)) {
        return;
    }

    const int previousItem = m_activeItemIndex;
    m_activeFooter = index;
    m_activeItemIndex = -1;
    m_activeItemId = 0;
    ClearHistoryHover();
    SetListSelection(m_historyList, -1);
    SetListSelection(m_pinsList, -1);
    InvalidateHistoryItem(previousItem, items, m_historyList, m_pinsList);
    InvalidateFooterButtons();
}

CListBox KeyboardHandler::ListForItem(int index, const std::vector<ClipboardItem>& items,
                                      CListBox historyList, CListBox pinsList) const {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return CListBox();
    return items[static_cast<size_t>(index)].pinned ? pinsList : historyList;
}

int KeyboardHandler::RowForItem(int index, const std::vector<ClipboardItem>& items,
                                CListBox historyList, CListBox pinsList) const {
    const CListBox list = ListForItem(index, items, historyList, pinsList);
    if (list.m_hWnd == nullptr) return -1;
    const int count = list.GetCount();
    for (int row = 0; row < count; ++row) {
        if (ItemIndexAtRow(list, row, items) == index) return row;
    }
    return -1;
}

int KeyboardHandler::ItemIndexAtRow(CListBox list, int row,
                                    const std::vector<ClipboardItem>& items) const {
    if (list.m_hWnd == nullptr || row < 0) return -1;
    const DWORD_PTR value = list.GetItemData(row);
    if (value == LB_ERR || static_cast<size_t>(value) >= items.size()) return -1;
    return static_cast<int>(value);
}

int KeyboardHandler::HistoryItemAtPoint(CListBox window, POINT point,
                                        CListBox historyList, CListBox pinsList,
                                        const std::vector<ClipboardItem>& items) const {
    if (window.m_hWnd != historyList.m_hWnd && window.m_hWnd != pinsList.m_hWnd) return -1;
    BOOL outside = FALSE;
    const UINT row = window.ItemFromPoint(point, outside);
    if (outside) return -1;
    return ItemIndexAtRow(window, static_cast<int>(row), items);
}

void KeyboardHandler::InvalidateHistoryItem(int index, const std::vector<ClipboardItem>& items,
                                            CListBox historyList, CListBox pinsList) {
    const int row = RowForItem(index, items, historyList, pinsList);
    if (row < 0) return;
    RECT rect{};
    CListBox list = ListForItem(index, items, historyList, pinsList);
    if (list.m_hWnd != nullptr && list.GetItemRect(row, &rect) != LB_ERR) {
        list.InvalidateRect(&rect, FALSE);
    }
}

void KeyboardHandler::SetListSelection(CListBox list, int row) const {
    if (list.m_hWnd == nullptr) return;
    list.SetCurSel(row);
}

void KeyboardHandler::InvalidateFooterButtons() const {
    for (CButton button : m_footerButtons) {
        if (button.m_hWnd != nullptr) {
            button.InvalidateRect(nullptr, FALSE);
        }
    }
}

void KeyboardHandler::SetActiveHistoryItem(int index, const std::vector<ClipboardItem>& items,
                                           CListBox historyList, CListBox pinsList,
                                           bool scrollIntoView) {
    if (index < 0 || static_cast<size_t>(index) >= items.size()) return;
    const int previous = m_activeItemIndex;
    const int previousFooter = m_activeFooter;
    m_activeItemIndex = index;
    m_activeItemId = items[index].id;
    m_activeFooter = -1;

    if (scrollIntoView) {
        SetListSelection(historyList, -1);
        SetListSelection(pinsList, -1);
        CListBox list = ListForItem(index, items, historyList, pinsList);
        if (list.m_hWnd != nullptr) {
            SetListSelection(list, RowForItem(index, items, historyList, pinsList));
        }
    }

    InvalidateHistoryItem(previous, items, historyList, pinsList);
    InvalidateHistoryItem(index, items, historyList, pinsList);
    if (previousFooter >= 0) {
        InvalidateFooterButtons();
    }
}

void KeyboardHandler::BeginHistoryMouseTracking() {
    TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, m_hoverList.m_hWnd, 0};
    ::TrackMouseEvent(&tracking);
}

void KeyboardHandler::OnHistoryMouseMove(CListBox window, POINT point,
                                        const std::vector<ClipboardItem>& items,
                                        CListBox historyList, CListBox pinsList,
                                        bool popupVisible) {
    if (!MouseCanSelect()) return;
    if (!popupVisible || (window.m_hWnd != historyList.m_hWnd && window.m_hWnd != pinsList.m_hWnd)) {
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
        if (m_activeFooter >= 0 || m_activeItemIndex != index) {
            SetActiveHistoryItem(index, items, historyList, pinsList, false);
            if (m_previewCallback && m_callbackContext) {
                m_previewCallback(m_callbackContext, item_id, false);
            }
        }
        return;
    }

    const int previous_index = m_hoveredItemIndex;
    m_hoveredItemIndex = index;
    m_hoveredItemId = item_id;
    SetActiveHistoryItem(index, items, historyList, pinsList, false);

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
                                                   CListBox historyList, CListBox pinsList,
                                                   bool popupVisible) {
    if (!popupVisible) return;
    POINT cursor{};
    if (!GetCursorPos(&cursor)) return;
    for (const CListBox& list : {historyList, pinsList}) {
        if (!list.IsWindowVisible()) continue;
        POINT point = cursor;
        list.ScreenToClient(&point);
        RECT rect{};
        list.GetClientRect(&rect);
        if (::PtInRect(&rect, point)) {
            OnHistoryMouseMove(list, point, items, historyList, pinsList, popupVisible);
            return;
        }
    }
    ClearHistoryHover();
}

void KeyboardHandler::ClearHistoryHover() {
    m_hoveredItemIndex = -1;
    m_hoveredItemId = 0;
    if (m_owner.m_hWnd != nullptr) {
        KillTimer(m_owner.m_hWnd, AppConstants::Timer::kPreview);
    }
}

void KeyboardHandler::NavigateHistoryFromSearch(bool forward,
                                               const std::vector<ClipboardItem>& items,
                                               CListBox historyList, CListBox pinsList,
                                               bool showFooter) {
    m_keyboardNavigating = true;
    GetCursorPos(&m_keyboardPointer);
    ClearHistoryHover();

    const int count = static_cast<int>(items.size());
    const int total = count + (showFooter ? AppConstants::UI::kFooterButtonCount : 0);
    if (!total) return;

    int current = m_activeFooter >= 0 ? count + m_activeFooter : m_activeItemIndex;
    const int target = std::clamp(current + (forward ? 1 : -1), 0, total - 1);

    if (target >= count) {
        SetActiveFooter(target - count, items);
    } else {
        SetActiveHistoryItem(target, items, historyList, pinsList);
        if (m_previewCallback && m_callbackContext) {
            m_previewCallback(m_callbackContext, m_activeItemId, true);
        }
    }
}

bool KeyboardHandler::HandlePopupKey(WPARAM key, const CEdit& search, const std::vector<ClipboardItem>& items) {
    if (m_searchCallback && search.m_hWnd != nullptr &&
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
        const bool shift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
        const bool forward = key == VK_DOWN || (key == VK_TAB && !shift);
        NavigateHistoryFromSearch(forward, items, m_historyList, m_pinsList, m_settings.show_footer);
        return true;
    }
    if ((ctrl && (key == VK_HOME || key == VK_END)) || key == VK_PRIOR || key == VK_NEXT) {
        m_keyboardNavigating = true;
        GetCursorPos(&m_keyboardPointer);
        ClearHistoryHover();
        if (!items.empty()) {
            const int target = (key == VK_HOME || key == VK_PRIOR) ? 0 : static_cast<int>(items.size()) - 1;
            SetActiveHistoryItem(target, items, m_historyList, m_pinsList);
            if (m_previewCallback && m_callbackContext) {
                m_previewCallback(m_callbackContext, m_activeItemId, true);
            }
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
