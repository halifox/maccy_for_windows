#include "HistoryListControls.h"

#include <windowsx.h>

#include <utility>

HistoryListControls::HistoryListControls()
    : m_historyList(this, 0),
      m_pinsList(this, 0) {}

void HistoryListControls::Configure(
    KeyboardHandler &keyboard_handler,
    HistoryRenderer &renderer,
    HWND search_window,
    std::vector<ClipboardItem> &items,
    std::wstring &search_query,
    bool &loading_list,
    bool &popup_visible,
    FooterUpdateCallback request_footer_update,
    PasteCallback paste_item,
    SelectionCallback selection_changed
) {
    m_keyboardHandler = &keyboard_handler;
    m_renderer = &renderer;
    m_search.Attach(search_window);
    m_items = &items;
    m_searchQuery = &search_query;
    m_loadingList = &loading_list;
    m_popupVisible = &popup_visible;
    m_requestFooterUpdate = std::move(request_footer_update);
    m_pasteItem = std::move(paste_item);
    m_selectionChanged = std::move(selection_changed);
}

void HistoryListControls::Shutdown() noexcept {
    m_keyboardHandler = nullptr;
    m_renderer = nullptr;
    m_search.Detach();
    m_items = nullptr;
    m_searchQuery = nullptr;
    m_loadingList = nullptr;
    m_popupVisible = nullptr;
    m_requestFooterUpdate = {};
    m_pasteItem = {};
    m_selectionChanged = {};
}

CListBox &HistoryListControls::CurrentHistoryList() noexcept {
    return m_historyList.GetCurrentMessage() != nullptr ? m_historyList : m_pinsList;
}

bool HistoryListControls::IsConfigured() const noexcept {
    return m_keyboardHandler != nullptr && m_renderer != nullptr && m_items != nullptr &&
        m_searchQuery != nullptr && m_loadingList != nullptr && m_popupVisible != nullptr;
}

LRESULT HistoryListControls::OnHistoryListMouseMove(
    UINT,
    WPARAM,
    LPARAM lParam,
    BOOL &handled
) {
    if (IsConfigured()) {
        CListBox &list = CurrentHistoryList();
        m_keyboardHandler->OnHistoryMouseMove(
            list.m_hWnd,
            POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
            *m_items,
            m_historyList,
            m_pinsList,
            *m_popupVisible
        );
    }
    handled = FALSE;
    return 0;
}

LRESULT HistoryListControls::OnHistoryListMouseLeave(UINT, WPARAM, LPARAM, BOOL &handled) {
    if (m_keyboardHandler != nullptr) {
        m_keyboardHandler->OnHistoryMouseLeave();
    }
    handled = FALSE;
    return 0;
}

LRESULT HistoryListControls::OnHistoryListKeyDown(
    UINT message,
    WPARAM key,
    LPARAM,
    BOOL &handled
) {
    if (!IsConfigured()) {
        handled = FALSE;
        return 0;
    }

    if (m_requestFooterUpdate) {
        m_requestFooterUpdate(message, key);
    }
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN) &&
        m_keyboardHandler->HandlePopupKey(key, m_search, *m_items)) {
        handled = TRUE;
        return 0;
    }
    handled = FALSE;
    return 0;
}

LRESULT HistoryListControls::OnHistoryListChar(
    UINT,
    WPARAM character,
    LPARAM,
    BOOL &handled
) {
    if (IsConfigured() && character >= 0x20 && character != 0x7f) {
        m_keyboardHandler->TypeToSearch(character, m_search);
        handled = TRUE;
    } else {
        handled = FALSE;
    }
    return 0;
}

LRESULT HistoryListControls::OnHistoryListButtonUp(
    UINT,
    WPARAM,
    LPARAM lParam,
    BOOL &handled
) {
    if (IsConfigured()) {
        CListBox &list = CurrentHistoryList();
        const int index = m_keyboardHandler->HistoryItemAtPoint(
            list.m_hWnd,
            POINT{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)},
            m_historyList,
            m_pinsList,
            *m_items
        );
        if (index >= 0 && m_pasteItem) {
            m_pasteItem(index);
        }
    }
    handled = TRUE;
    return 0;
}

LRESULT HistoryListControls::OnHistoryListScroll(
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL &handled
) {
    CListBox &list = CurrentHistoryList();
    const LRESULT result = list.DefWindowProc(message, wParam, lParam);
    if (IsConfigured()) {
        m_keyboardHandler->UpdateHistoryHoverFromCursor(
            *m_items,
            m_historyList,
            m_pinsList,
            *m_popupVisible
        );
    }
    handled = TRUE;
    return result;
}

LRESULT HistoryListControls::OnHistoryListSelectionChanged(
    WORD,
    WORD id,
    HWND,
    BOOL &handled
) {
    handled = TRUE;
    if (!IsConfigured() || *m_loadingList) {
        return 0;
    }

    CListBox &list = id == IDC_HISTORY_PINS ? m_pinsList : m_historyList;
    const int selected = list.GetCurSel();
    if (selected == LB_ERR) {
        return 0;
    }

    const int selected_index = m_keyboardHandler->ItemIndexAtRow(list, selected, *m_items);
    if (selected_index < 0) {
        return 0;
    }

    const bool selected_by_mouse =
        m_keyboardHandler->GetHoveredItemIndex() == selected_index;
    m_keyboardHandler->SetActiveHistoryItem(
        selected_index,
        *m_items,
        m_historyList,
        m_pinsList,
        !selected_by_mouse
    );
    if (!selected_by_mouse) {
        m_keyboardHandler->ClearHistoryHover();
        if (m_selectionChanged) {
            m_selectionChanged(m_keyboardHandler->GetActiveItemId());
        }
    }
    return 0;
}

LRESULT HistoryListControls::OnMeasureItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    auto *measure = reinterpret_cast<MEASUREITEMSTRUCT *>(lParam);
    if (measure == nullptr ||
        (measure->CtlID != IDC_HISTORY_LIST && measure->CtlID != IDC_HISTORY_PINS)) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    measure->itemHeight = AppConstants::UI::kHistoryItemHeight;
    return 0;
}

LRESULT HistoryListControls::OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    auto *draw = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
    if (!IsConfigured() || draw == nullptr || draw->CtlType != ODT_LISTBOX ||
        (draw->CtlID != IDC_HISTORY_LIST && draw->CtlID != IDC_HISTORY_PINS)) {
        handled = FALSE;
        return 0;
    }

    handled = TRUE;
    m_renderer->DrawHistoryItem(
        draw,
        *m_items,
        m_keyboardHandler->GetActiveItemIndex(),
        m_keyboardHandler->GetActiveFooter(),
        *m_searchQuery
    );
    return 0;
}
