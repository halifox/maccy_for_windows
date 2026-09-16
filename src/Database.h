#pragma once

#include "PlatformConfig.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <windows.h>
#include <sqlite3.h>

class ClipboardPayloadStore;

// A clipboard item is represented as a small metadata record until it is
// previewed or pasted. The complete clipboard formats are kept in the
// filesystem and loaded on demand.
struct ClipboardFormatData {
    std::wstring name;
    UINT format = 0;
    std::vector<unsigned char> bytes;
};

struct ClipboardCapture {
    std::wstring fingerprint;
    std::wstring title;
    std::wstring preview;
    std::wstring application;
    bool has_text = false;
    bool has_image = false;
    bool has_files = false;
    std::vector<ClipboardFormatData> data;
};

struct ClipboardItem {
    sqlite3_int64 id = 0;
    std::wstring title;
    std::wstring preview;
    std::wstring application;
    std::wstring pin;
    sqlite3_int64 first_copied_at = 0;
    sqlite3_int64 copied_at = 0;
    int copy_count = 1;
    bool pinned = false;
    bool has_text = false;
    bool has_image = false;
    bool has_files = false;
    std::vector<ClipboardFormatData> data;
};

enum class DatabaseList {
    IgnoredApplications,
    IgnoredFormats,
    IgnoredRegexps,
};

class Database {
public:
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

    std::vector<std::wstring> GetList(DatabaseList list) const;
    void ReplaceList(DatabaseList list, const std::vector<std::wstring> &values) const;
    void ResetIgnoredFormats() const;

    void SaveClipboard(const ClipboardCapture &capture, int max_unpinned) const;
    void TrimUnpinned(int max_unpinned) const;
    std::vector<ClipboardItem> SearchHistory(
        std::wstring_view query,
        int search_mode,
        int sort_by,
        bool pins_at_bottom
    ) const;
    std::optional<ClipboardItem> GetItem(sqlite3_int64 id, bool load_payload = false) const;
    std::vector<ClipboardItem> GetPinnedItems(bool load_payload = false) const;

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
    void Exec(std::string_view sql) const;
    void InitializeHistorySchema();
    sqlite3_int64 CountRows(const char *table) const;
    const char *ListTable(DatabaseList list) const;
    std::vector<sqlite3_int64> SelectItemIds(std::string_view condition) const;
    void RemovePayloads(const std::vector<sqlite3_int64>& ids) const;
    void LoadPayload(ClipboardItem &item) const;

    sqlite3 *m_db = nullptr;
    std::filesystem::path m_path;
    std::unique_ptr<ClipboardPayloadStore> m_payloadStore;
};
