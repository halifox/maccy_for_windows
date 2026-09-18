#pragma once

#include "ClipboardData.h"
#include "PlatformConfig.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <sqlite3.h>

class Database {
public:
    using SearchCancellation = std::function<bool()>;

    class Transaction {
    public:
        explicit Transaction(const Database& database);
        ~Transaction();

        Transaction(const Transaction&) = delete;
        Transaction& operator=(const Transaction&) = delete;
        Transaction(Transaction&& other) noexcept;
        Transaction& operator=(Transaction&& other) noexcept;

        void Commit();

    private:
        const Database* m_database = nullptr;
        bool m_committed = false;
    };

    explicit Database(const std::filesystem::path &path);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    Transaction BeginTransaction() const { return Transaction(*this); }

    std::optional<std::wstring> GetSetting(std::wstring_view key) const;
    void SetSetting(std::wstring_view key, std::wstring_view value) const;

    std::vector<std::wstring> GetList(IgnoreListKind list) const;
    void ReplaceList(IgnoreListKind list, const std::vector<std::wstring> &values) const;
    void ResetIgnoredFormats() const;

    void SaveClipboard(const ClipboardSnapshot &capture, int max_unpinned) const;
    void TrimUnpinned(int max_unpinned) const;
    std::vector<ClipboardItem> SearchHistory(
        std::wstring_view query,
        int search_mode,
        int sort_by,
        bool pins_at_bottom,
        SearchCancellation is_cancelled = {}
    ) const;
    std::optional<ClipboardItem> GetItem(
        sqlite3_int64 id,
        PayloadMode payload_mode = PayloadMode::Metadata
    ) const;
    std::vector<ClipboardItem> GetPinnedItems(
        PayloadMode payload_mode = PayloadMode::Metadata
    ) const;

    void MarkCopied(sqlite3_int64 id) const;
    void DeleteItem(sqlite3_int64 id) const;
    void DeleteUnpinned() const;
    void DeleteAll() const;
    void TogglePin(sqlite3_int64 id, std::wstring_view pin_key, bool pinned) const;
    void UpdatePinnedItem(
        sqlite3_int64 id,
        std::wstring_view pin_key,
        std::wstring_view title,
        std::wstring_view text
    ) const;
    void UpdatePinnedMetadata(
        sqlite3_int64 id,
        std::wstring_view pin_key,
        std::wstring_view title
    ) const;
    void RegenerateTitles(bool show_special_symbols) const;

    sqlite3_int64 CountItems() const;
    std::uintmax_t StorageBytes() const;
    const std::filesystem::path &Path() const noexcept { return m_path; }

private:
    struct SearchDocument {
        sqlite3_int64 id = 0;
        std::wstring body;
        std::wstring paths;
    };

    void Exec(std::string_view sql) const;
    void CreateHistoryTables();
    sqlite3_int64 CountRows(const char *table) const;
    const char *ListTable(IgnoreListKind list) const;
    void LoadPayload(ClipboardItem &item, PayloadMode payload_mode) const;
    void ReplaceFormats(sqlite3_int64 item_id, const std::vector<ClipboardFormatData> &data) const;
    void ReplaceSearchIndex(
        sqlite3_int64 item_id,
        std::wstring_view body,
        std::wstring_view paths
    ) const;
    void RebuildSearchDocuments() const;
    std::vector<sqlite3_int64> SearchIndexIds(
        std::wstring_view query,
        const SearchCancellation &is_cancelled = {}
    ) const;
    void ForEachRawSearchDocument(
        const std::function<void(const SearchDocument &)> &callback,
        const SearchCancellation &is_cancelled
    ) const;
    void ForEachSearchDocument(
        const std::function<void(const SearchDocument &)> &callback,
        const SearchCancellation &is_cancelled = {}
    ) const;

    sqlite3 *m_db = nullptr;
    std::filesystem::path m_path;
};
