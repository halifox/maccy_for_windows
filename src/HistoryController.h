#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Database.h"
#include "HistoryRenderer.h"
#include "HistoryView.h"
#include "KeyboardHandler.h"
#include "Settings.h"

class HistoryController {
public:
    using RefreshCallback = void (*)(void* context, std::wstring_view query);

    explicit HistoryController(Database& database) : m_database(database) {}

    HistoryController(const HistoryController&) = delete;
    HistoryController& operator=(const HistoryController&) = delete;

    void SetRefreshCallback(void* context, RefreshCallback callback) noexcept {
        m_refreshContext = context;
        m_refreshCallback = callback;
    }

    std::vector<ClipboardItem> Query(std::wstring_view query, const AppSettings& settings) const;
    sqlite3_int64 Refresh(HistoryView& view,
                          HistoryRenderer& renderer,
                          KeyboardHandler& keyboard,
                          std::vector<ClipboardItem>& items,
                          std::wstring_view query,
                          const AppSettings& settings,
                          sqlite3_int64 previousId,
                          int previousIndex,
                          bool sameQuery) const;
    void RequestRefresh(std::wstring_view query) const;
    bool TogglePin(const ClipboardItem& item, const AppSettings& settings) const;
    void DeleteItem(sqlite3_int64 id) const;
    void Clear(bool all) const;
    std::wstring NextPinKey(const AppSettings& settings) const;

private:
    Database& m_database;
    void* m_refreshContext = nullptr;
    RefreshCallback m_refreshCallback = nullptr;
};
