#include "HistoryView.h"

#include "HistoryRenderer.h"
#include "KeyboardHandler.h"

#include <algorithm>

void HistoryView::Initialize(HWND historyList, HWND pinsList) noexcept {
    m_historyList = historyList;
    m_pinsList = pinsList;
}

void HistoryView::Bind(HistoryRenderer& renderer,
                      KeyboardHandler& keyboard,
                      std::wstring_view query,
                      sqlite3_int64 previousId,
                      int previousIndex,
                      bool sameQuery) {
    m_loading = true;
    for (HWND list : {m_historyList, m_pinsList}) {
        if (list == nullptr) {
            continue;
        }
        SendMessageW(list, WM_SETREDRAW, FALSE, 0);
        SendMessageW(list, LB_RESETCONTENT, 0, 0);
    }

    int selected = -1;
    for (size_t index = 0; index < m_items.size(); ++index) {
        const auto& item = m_items[index];
        HWND list = item.pinned ? m_pinsList : m_historyList;
        const std::wstring display = renderer.DisplayText(item);
        const LRESULT row = SendMessageW(list, LB_ADDSTRING, 0,
                                         reinterpret_cast<LPARAM>(display.c_str()));
        SendMessageW(list, LB_SETITEMDATA, row, index);
        if (item.id == previousId) {
            selected = static_cast<int>(index);
        }
    }

    if (selected < 0 && !m_items.empty()) {
        if (previousId != 0 && sameQuery) {
            selected = std::clamp(previousIndex, 0, static_cast<int>(m_items.size()) - 1);
        } else {
            selected = 0;
            if (query.empty()) {
                const auto it = std::find_if(m_items.begin(), m_items.end(),
                                             [](const auto& item) { return !item.pinned; });
                if (it != m_items.end()) {
                    selected = static_cast<int>(it - m_items.begin());
                }
            }
        }
    }
    if (selected >= 0) {
        keyboard.SetActiveHistoryItem(selected, m_items, m_historyList, m_pinsList);
    }

    m_loading = false;
    for (HWND list : {m_historyList, m_pinsList}) {
        if (list != nullptr) {
            SendMessageW(list, WM_SETREDRAW, TRUE, 0);
        }
    }
}

int HistoryView::ItemIndexAtRow(HWND list, int row) const {
    if (list == nullptr || row < 0) {
        return -1;
    }
    const LRESULT value = SendMessageW(list, LB_GETITEMDATA, row, 0);
    if (value == LB_ERR || static_cast<size_t>(value) >= m_items.size()) {
        return -1;
    }
    return static_cast<int>(value);
}

int HistoryView::ItemIndexAtPoint(HWND list, POINT point) const {
    if (list != m_historyList && list != m_pinsList) {
        return -1;
    }
    const LRESULT hit = SendMessageW(list, LB_ITEMFROMPOINT, 0, MAKELPARAM(point.x, point.y));
    if (HIWORD(hit) != 0) {
        return -1;
    }
    return ItemIndexAtRow(list, LOWORD(hit));
}

HWND HistoryView::ListForItem(int index) const {
    if (index < 0 || static_cast<size_t>(index) >= m_items.size()) {
        return nullptr;
    }
    return m_items[static_cast<size_t>(index)].pinned ? m_pinsList : m_historyList;
}

int HistoryView::RowForItem(int index) const {
    const HWND list = ListForItem(index);
    if (list == nullptr) {
        return -1;
    }
    const int count = static_cast<int>(SendMessageW(list, LB_GETCOUNT, 0, 0));
    for (int row = 0; row < count; ++row) {
        if (ItemIndexAtRow(list, row) == index) {
            return row;
        }
    }
    return -1;
}
