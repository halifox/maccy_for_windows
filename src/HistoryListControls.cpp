#include "HistoryListControls.h"

#include "SearchHeaderLayout.h"
#include "UiFont.h"

#include <windowsx.h>

#include <algorithm>
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
    SelectionCallback selection_changed,
    int image_max_height
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
    m_imageMaxHeight = image_max_height;
    RefreshItemHeights();
}

int HistoryListControls::ItemHeightPixels() const noexcept {
    const HWND list = m_historyList.m_hWnd != nullptr ? m_historyList.m_hWnd : m_pinsList.m_hWnd;
    const UINT dpi = UiFont::DpiForWindow(list);
    return SearchHeaderLayout::Scale(AppConstants::UI::kHistoryItemHeight, dpi);
}

int HistoryListControls::RowHeightPixels(const CListBox &list, int row) const noexcept {
    const UINT dpi = UiFont::DpiForWindow(list.m_hWnd);
    const int base_height = SearchHeaderLayout::Scale(AppConstants::UI::kHistoryItemHeight, dpi);
    if (row < 0 || m_items == nullptr) {
        return base_height;
    }

    LRESULT data = list.GetItemData(row);
    if (data == LB_ERR) {
        const auto pending = m_pendingMeasureItems.find(list.m_hWnd);
        if (pending == m_pendingMeasureItems.end() ||
            static_cast<size_t>(row) >= pending->second.size()) {
            return base_height;
        }
        data = pending->second[static_cast<size_t>(row)];
    }
    if (static_cast<size_t>(data) >= m_items->size() ||
        !(*m_items)[static_cast<size_t>(data)].has_image ||
        (*m_items)[static_cast<size_t>(data)].has_files) {
        return base_height;
    }

    const int max_height = SearchHeaderLayout::Scale(m_imageMaxHeight, dpi);
    const int padding = SearchHeaderLayout::Scale(8, dpi);
    return std::max(base_height, max_height + padding);
}

void HistoryListControls::RefreshItemHeights() noexcept {
    UpdateAllItemHeights();
}

int HistoryListControls::TotalHeightPixels(const CListBox &list) const noexcept {
    int height = 0;
    for (int row = 0; row < list.GetCount(); ++row) {
        height += RowHeightPixels(list, row);
    }
    return height;
}

bool HistoryListControls::DeferListRefresh() noexcept {
    if (!m_wheelGesture) {
        return false;
    }
    m_refreshAfterWheel = true;
    return true;
}

void HistoryListControls::InvalidateLists() {
    for (CListBox *list : {&m_historyList, &m_pinsList}) {
        if (list->m_hWnd != nullptr) {
            list->Invalidate(FALSE);
        }
    }
}

void HistoryListControls::FinishWheelGesture() {
    m_wheelGesture = false;
    if (!m_refreshAfterWheel) {
        return;
    }
    m_refreshAfterWheel = false;
    InvalidateLists();
}

int HistoryListControls::WheelPageRows(const CListBox &list) const {
    RECT client{};
    if (list.m_hWnd == nullptr || !list.GetClientRect(&client) || client.bottom <= 0) {
        return 1;
    }
    const int count = list.GetCount();
    int rows = 0;
    int y = 0;
    for (int row = list.GetTopIndex(); row < count && y < client.bottom; ++row) {
        const int height = std::max(1, list.GetItemHeight(row));
        if (y > 0 && y + height > client.bottom) {
            break;
        }
        y += height;
        ++rows;
    }
    return std::max(rows, 1);
}

int ItemSpanPixels(const CListBox &list, int from, int to) {
    int pixels = 0;
    const int start = std::min(from, to);
    const int end = std::max(from, to);
    for (int row = start; row < end; ++row) {
        pixels += std::max(1, list.GetItemHeight(row));
    }
    return pixels;
}

int MaximumTopIndex(const CListBox &list) {
    const int count = list.GetCount();
    RECT client{};
    if (count <= 0 || !list.GetClientRect(&client) || client.bottom <= 0) {
        return 0;
    }
    // Largest top index whose rows through the last item still fit. Stopping on
    // the row that overflows leaves the last item clipped at the bottom.
    int used = 0;
    for (int row = count - 1; row >= 0; --row) {
        const int height = std::max(1, list.GetItemHeight(row));
        if (used + height > client.bottom) {
            return std::min(row + 1, count - 1);
        }
        used += height;
    }
    return 0;
}

void SyncVerticalScroll(const CListBox &list) {
    if (list.m_hWnd == nullptr) {
        return;
    }
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_POS;
    info.nPos = list.GetTopIndex();
    ::SetScrollInfo(list.m_hWnd, SB_VERT, &info, TRUE);
}

void HistoryListControls::ScrollWheel(CListBox &list, int delta) {
    if (list.m_hWnd == nullptr || delta == 0) {
        return;
    }
    m_wheelRemainder += delta;
    UINT lines_per_notch = 3;
    ::SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &lines_per_notch, 0);
    if (lines_per_notch == 0) {
        m_wheelRemainder = 0;
        return;
    }
    const int notches = m_wheelRemainder / WHEEL_DELTA;
    m_wheelRemainder -= notches * WHEEL_DELTA;
    if (notches == 0 || list.GetCount() <= 0) {
        return;
    }

    const int step = lines_per_notch == WHEEL_PAGESCROLL
        ? WheelPageRows(list)
        : static_cast<int>(lines_per_notch);
    const int top = list.GetTopIndex();
    int target = top - notches * std::max(step, 1);
    const int maximum_top = MaximumTopIndex(list);
    if (target < 0) {
        target = 0;
    } else if (target > maximum_top) {
        target = maximum_top;
    }
    if (target == top) {
        return;
    }

    const int pixels = ItemSpanPixels(list, top, target);
    // Wheel up reveals earlier items, so the existing rows move downward.
    const int dy = target > top ? -pixels : pixels;

    // Keep the bits that are still on screen and paint only the new strip.
    // The variable-height listbox otherwise repaints every row on each notch.
    list.SetRedraw(FALSE);
    list.SetTopIndex(target);
    list.SetRedraw(TRUE);
    list.ValidateRect(nullptr);
    SyncVerticalScroll(list);

    RECT client{};
    list.GetClientRect(&client);
    ::ScrollWindowEx(
        list.m_hWnd,
        0,
        dy,
        &client,
        &client,
        nullptr,
        nullptr,
        SW_INVALIDATE | SW_ERASE
    );
    list.UpdateWindow();
}

void HistoryListControls::UpdateItemHeight(sqlite3_int64 item_id) noexcept {
    if (m_wheelGesture) {
        m_refreshAfterWheel = true;
        return;
    }
    if (m_items == nullptr || item_id == 0) {
        return;
    }
    const auto item = std::find_if(m_items->begin(), m_items->end(), [item_id](const ClipboardItem &value) {
        return value.id == item_id;
    });
    if (item == m_items->end()) {
        return;
    }
    const int item_index = static_cast<int>(item - m_items->begin());
    for (CListBox *list : {&m_historyList, &m_pinsList}) {
        for (int row = 0; row < list->GetCount(); ++row) {
            if (list->GetItemData(row) == static_cast<DWORD_PTR>(item_index)) {
                // Image rows already reserve their height when the list is filled.
                // SetItemHeight again makes a variable-height listbox shift its scroll offset.
                const auto height = static_cast<UINT>(RowHeightPixels(*list, row));
                if (list->GetItemHeight(row) != static_cast<int>(height)) {
                    list->SetItemHeight(row, height);
                }
                RECT row_rect{};
                if (list->GetItemRect(row, &row_rect) != LB_ERR) {
                    list->InvalidateRect(&row_rect, FALSE);
                }
                break;
            }
        }
    }
}

void HistoryListControls::UpdateAllItemHeights() noexcept {
    for (CListBox *list : {&m_historyList, &m_pinsList}) {
        for (int row = 0; row < list->GetCount(); ++row) {
            list->SetItemHeight(row, static_cast<UINT>(RowHeightPixels(*list, row)));
        }
    }
}

void HistoryListControls::SetPendingMeasureItems(HWND list, std::vector<int> item_indices) {
    if (item_indices.empty()) {
        m_pendingMeasureItems.erase(list);
        return;
    }
    m_pendingMeasureItems.insert_or_assign(list, std::move(item_indices));
}

void HistoryListControls::ResetScrollPositions() noexcept {
    for (CListBox *list : {&m_historyList, &m_pinsList}) {
        if (list->m_hWnd != nullptr) {
            list->SetTopIndex(0);
        }
    }
}

void HistoryListControls::SetImageMaxHeight(int image_max_height) noexcept {
    m_imageMaxHeight = std::clamp(image_max_height, 1, 200);
    UpdateAllItemHeights();
}

void HistoryListControls::Shutdown() noexcept {
    for (CListBox *list : {&m_historyList, &m_pinsList}) {
        if (list->m_hWnd != nullptr) {
            list->KillTimer(AppConstants::Timer::kHistoryHover);
        }
    }
    m_pendingMeasureItems.clear();
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
    CContainedWindowT<CListBox> &listWindow =
        m_historyList.GetCurrentMessage() != nullptr ? m_historyList : m_pinsList;
    if (message == WM_MOUSEWHEEL) {
        m_wheelGesture = true;
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        MSG pending{};
        while (::PeekMessageW(
            &pending,
            listWindow.m_hWnd,
            WM_MOUSEWHEEL,
            WM_MOUSEWHEEL,
            PM_REMOVE
        )) {
            delta += GET_WHEEL_DELTA_WPARAM(pending.wParam);
        }
        ScrollWheel(listWindow, delta);
        if (IsConfigured()) {
            listWindow.SetTimer(AppConstants::Timer::kHistoryHover, 50, nullptr);
        }
        handled = TRUE;
        return 0;
    }
    const LRESULT result = listWindow.DefWindowProc(message, wParam, lParam);
    if (IsConfigured()) {
        // The row under a stationary cursor changes while the list moves.
        // Update it once the wheel settles instead of on every notch.
        listWindow.SetTimer(AppConstants::Timer::kHistoryHover, 50, nullptr);
    }
    handled = TRUE;
    return result;
}

LRESULT HistoryListControls::OnHistoryListTimer(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    if (wParam != AppConstants::Timer::kHistoryHover) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    CContainedWindowT<CListBox> &listWindow =
        m_historyList.GetCurrentMessage() != nullptr ? m_historyList : m_pinsList;
    listWindow.KillTimer(AppConstants::Timer::kHistoryHover);
    FinishWheelGesture();
    if (IsConfigured()) {
        m_keyboardHandler->UpdateHistoryHoverFromCursor(
            *m_items,
            m_historyList,
            m_pinsList,
            *m_popupVisible
        );
    }
    return 0;
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
    CListBox &list = measure->CtlID == IDC_HISTORY_PINS ? m_pinsList : m_historyList;
    measure->itemHeight = static_cast<UINT>(RowHeightPixels(list, static_cast<int>(measure->itemID)));
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
