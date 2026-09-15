#pragma once

#include <string_view>
#include <vector>

#include "Database.h"

class HistoryRenderer;
class KeyboardHandler;

class HistoryView {
public:
    explicit HistoryView(std::vector<ClipboardItem>& items) : m_items(items) {}

    void Initialize(HWND historyList, HWND pinsList) noexcept;
    void Bind(HistoryRenderer& renderer,
              KeyboardHandler& keyboard,
              std::wstring_view query,
              sqlite3_int64 previousId,
              int previousIndex,
              bool sameQuery);

    HWND HistoryList() const noexcept { return m_historyList; }
    HWND PinsList() const noexcept { return m_pinsList; }
    bool IsLoading() const noexcept { return m_loading; }

    int ItemIndexAtRow(HWND list, int row) const;
    int ItemIndexAtPoint(HWND list, POINT point) const;
    HWND ListForItem(int index) const;
    int RowForItem(int index) const;

private:
    std::vector<ClipboardItem>& m_items;
    HWND m_historyList = nullptr;
    HWND m_pinsList = nullptr;
    bool m_loading = false;
};
