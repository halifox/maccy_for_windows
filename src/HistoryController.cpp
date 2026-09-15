#include "HistoryController.h"

#include "PinKeys.h"

std::vector<ClipboardItem> HistoryController::Query(std::wstring_view query, const AppSettings& settings) const {
    return m_database.SearchHistory(
        query,
        static_cast<int>(settings.search_mode),
        settings.sort_by,
        settings.pin_to == PinPosition::Bottom
    );
}

sqlite3_int64 HistoryController::Refresh(HistoryView& view,
                                          HistoryRenderer& renderer,
                                          KeyboardHandler& keyboard,
                                          std::vector<ClipboardItem>& items,
                                          std::wstring_view query,
                                          const AppSettings& settings,
                                          sqlite3_int64 previousId,
                                          int previousIndex,
                                          bool sameQuery) const {
    items = Query(query, settings);
    view.Bind(renderer, keyboard, query, previousId, previousIndex, sameQuery);
    return keyboard.GetActiveItemId();
}

void HistoryController::RequestRefresh(std::wstring_view query) const {
    if (m_refreshCallback != nullptr && m_refreshContext != nullptr) {
        m_refreshCallback(m_refreshContext, query);
    }
}

bool HistoryController::TogglePin(const ClipboardItem& item, const AppSettings& settings) const {
    if (item.pinned) {
        m_database.TogglePin(item.id, {}, false);
        return true;
    }
    const std::wstring key = NextPinKey(settings);
    if (key.empty()) {
        return false;
    }
    m_database.TogglePin(item.id, key, true);
    return true;
}

void HistoryController::DeleteItem(sqlite3_int64 id) const {
    m_database.DeleteItem(id);
}

void HistoryController::Clear(bool all) const {
    if (all) {
        m_database.DeleteAll();
    } else {
        m_database.DeleteUnpinned();
    }
}

std::wstring HistoryController::NextPinKey(const AppSettings& settings) const {
    return PinKeyPolicy::Next(m_database.SearchHistory({}, 0, 0, false), settings);
}
