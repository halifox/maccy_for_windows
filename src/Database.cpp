#include "Database.h"

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <cstring>
#include <limits>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::runtime_error MakeSqliteError(sqlite3 *db, const char *operation) {
    std::string message = operation;
    if (db != nullptr) {
        message += ": ";
        message += sqlite3_errmsg(db);
    }
    return std::runtime_error(message);
}

class Statement {
public:
    Statement(sqlite3 *db, const char *sql) : m_db(db) {
        const int result = sqlite3_prepare_v2(m_db, sql, -1, &m_statement, nullptr);
        if (result != SQLITE_OK) {
            throw MakeSqliteError(m_db, "Unable to prepare SQLite statement");
        }
    }

    ~Statement() {
        if (m_statement != nullptr) {
            sqlite3_finalize(m_statement);
        }
    }

    Statement(const Statement &) = delete;
    Statement &operator=(const Statement &) = delete;

    sqlite3_stmt *get() const noexcept { return m_statement; }

private:
    sqlite3 *m_db = nullptr;
    sqlite3_stmt *m_statement = nullptr;
};

void CheckSqliteResult(sqlite3 *db, int result, const char *operation) {
    if (result != SQLITE_OK) {
        throw MakeSqliteError(db, operation);
    }
}

void BindText16(sqlite3 *db, sqlite3_stmt *statement, int index, std::wstring_view text) {
    if (text.size() > static_cast<size_t>(std::numeric_limits<int>::max() / sizeof(wchar_t))) {
        throw std::runtime_error("Text value is too large");
    }

    const int byte_count = static_cast<int>(text.size() * sizeof(wchar_t));
    CheckSqliteResult(
        db,
        sqlite3_bind_text16(statement, index, text.data(), byte_count, SQLITE_TRANSIENT),
        "Unable to bind SQLite text"
    );
}

void BindBlob(sqlite3 *db, sqlite3_stmt *statement, int index, const std::vector<unsigned char> &bytes) {
    if (bytes.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("Clipboard format is too large");
    }

    CheckSqliteResult(
        db,
        sqlite3_bind_blob(
            statement,
            index,
            bytes.empty() ? nullptr : bytes.data(),
            static_cast<int>(bytes.size()),
            SQLITE_TRANSIENT
        ),
        "Unable to bind SQLite blob"
    );
}

std::wstring ColumnText16(sqlite3_stmt *statement, int column) {
    const auto *text = static_cast<const wchar_t *>(sqlite3_column_text16(statement, column));
    const int bytes = sqlite3_column_bytes16(statement, column);
    if (text == nullptr || bytes <= 0) {
        return {};
    }
    return std::wstring(text, static_cast<size_t>(bytes) / sizeof(wchar_t));
}

sqlite3_int64 CurrentUnixMilliseconds() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

bool TableExists(sqlite3 *db, const char *table) {
    Statement statement(
        db,
        "SELECT 1 FROM sqlite_master WHERE type = 'table' AND name = ?1 LIMIT 1;"
    );
    CheckSqliteResult(
        db,
        sqlite3_bind_text(statement.get(), 1, table, -1, SQLITE_TRANSIENT),
        "Unable to bind table name"
    );
    return sqlite3_step(statement.get()) == SQLITE_ROW;
}

std::wstring Lower(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (const wchar_t character : value) {
        result.push_back(static_cast<wchar_t>(std::towlower(character)));
    }
    return result;
}

std::wstring TrimWhitespace(std::wstring value) {
    const auto is_space = [](wchar_t character) {
        return std::iswspace(character) != 0;
    };
    const auto first = std::find_if_not(value.begin(), value.end(), is_space);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    if (first >= last) {
        return {};
    }
    return std::wstring(first, last);
}

const std::vector<std::wstring> &DefaultIgnoredFormats() {
    static const std::vector<std::wstring> values = {
        L"Pasteboard generator type",
        L"com.agilebits.onepassword",
        L"com.typeit4me.clipping",
        L"de.petermaurer.TransientPasteboardType",
        L"net.antelle.keeweb",
    };
    return values;
}

std::wstring MakeTitle(std::wstring value, bool show_special_symbols) {
    value.resize(std::min<size_t>(value.size(), 1000));
    if (!show_special_symbols) {
        return TrimWhitespace(std::move(value));
    }

    size_t leading = 0;
    while (leading < value.size() && value[leading] == L' ') {
        value[leading] = L'\x00b7';
        ++leading;
    }
    size_t trailing = value.size();
    while (trailing > 0 && value[trailing - 1] == L' ') {
        value[trailing - 1] = L'\x00b7';
        --trailing;
    }

    std::wstring result;
    result.reserve(value.size() + 8);
    for (const wchar_t character : value) {
        switch (character) {
        case L'\r':
            break;
        case L'\n':
            result += L'\x23ce';
            break;
        case L'\t':
            result += L'\x21e5';
            break;
        default:
            result += character;
            break;
        }
    }
    return result;
}

std::wstring StoredPreview(std::wstring value) {
    constexpr size_t kStoredPreviewCharacters = 4096;
    value.resize(std::min(value.size(), kStoredPreviewCharacters));
    return value;
}

bool ContainsExact(std::wstring_view haystack, std::wstring_view needle) {
    if (needle.empty()) {
        return true;
    }
    return Lower(haystack).find(Lower(needle)) != std::wstring::npos;
}

bool MatchRegex(std::wstring_view text, std::wstring_view pattern) {
    try {
        const std::wregex expression{std::wstring(pattern)};
        const std::wstring value(text);
        return std::regex_search(value, expression);
    } catch (const std::regex_error &) {
        return false;
    }
}

std::optional<double> FuzzyScore(std::wstring_view text, std::wstring_view pattern) {
    if (pattern.empty()) {
        return 0.0;
    }

    const std::wstring lower_text = Lower(text);
    const std::wstring lower_pattern = Lower(pattern);
    size_t text_index = 0;
    size_t last_match = 0;
    size_t gaps = 0;

    for (const wchar_t expected : lower_pattern) {
        const size_t found = lower_text.find(expected, text_index);
        if (found == std::wstring::npos) {
            return std::nullopt;
        }
        if (found > last_match && text_index != 0) {
            gaps += found - last_match;
        }
        last_match = found;
        text_index = found + 1;
    }

    return static_cast<double>(gaps) /
        static_cast<double>(std::max<size_t>(1, lower_text.size()));
}

} // namespace

Database::Database(const std::filesystem::path &path) : m_path(path) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }

    const int result = sqlite3_open16(path.c_str(), &m_db);
    if (result != SQLITE_OK) {
        const auto error = MakeSqliteError(m_db, "Unable to open SQLite database");
        sqlite3_close(m_db);
        m_db = nullptr;
        throw error;
    }

    try {
        Exec("PRAGMA foreign_keys = ON;");
        Exec("PRAGMA journal_mode = WAL;");
        Exec("PRAGMA synchronous = NORMAL;");
        Exec("PRAGMA temp_store = MEMORY;");
        Exec("PRAGMA busy_timeout = 1000;");
        Exec(
            "CREATE TABLE IF NOT EXISTS app_state ("
            "key TEXT PRIMARY KEY,"
            "value TEXT NOT NULL"
            ");"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS ignored_apps ("
            "value TEXT PRIMARY KEY"
            ");"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS ignored_formats ("
            "value TEXT PRIMARY KEY"
            ");"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS ignored_regexps ("
            "value TEXT PRIMARY KEY"
            ");"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS history_items ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "fingerprint TEXT NOT NULL UNIQUE,"
            "title TEXT NOT NULL DEFAULT '',"
            "content TEXT NOT NULL DEFAULT '',"
            "application TEXT NOT NULL DEFAULT '',"
            "pin TEXT,"
            "pinned INTEGER NOT NULL DEFAULT 0,"
            "first_copied_at INTEGER NOT NULL,"
            "copied_at INTEGER NOT NULL,"
            "copy_count INTEGER NOT NULL DEFAULT 1,"
            "title_custom INTEGER NOT NULL DEFAULT 0,"
            "has_text INTEGER NOT NULL DEFAULT 0,"
            "has_image INTEGER NOT NULL DEFAULT 0,"
            "has_files INTEGER NOT NULL DEFAULT 0"
            ");"
        );
        Exec(
            "CREATE INDEX IF NOT EXISTS idx_history_items_copied_at "
            "ON history_items(copied_at DESC);"
        );
        Exec(
            "CREATE TABLE IF NOT EXISTS history_data ("
            "item_id INTEGER NOT NULL,"
            "format_name TEXT NOT NULL,"
            "format_id INTEGER NOT NULL,"
            "data BLOB NOT NULL,"
            "PRIMARY KEY(item_id, format_name),"
            "FOREIGN KEY(item_id) REFERENCES history_items(id) ON DELETE CASCADE"
            ");"
        );
        MigrateLegacyHistory();
        if (!GetSetting(L"schema.compactPreview_v1")) {
            Exec("UPDATE history_items SET content = substr(content, 1, 4096) WHERE length(content) > 4096;");
            SetSetting(L"schema.compactPreview_v1", L"1");
        }
        if (!GetSetting(L"defaults.ignoredFormatsInitialized")) {
            ReplaceList(DatabaseList::IgnoredFormats, DefaultIgnoredFormats());
            SetSetting(L"defaults.ignoredFormatsInitialized", L"1");
        }
    } catch (...) {
        sqlite3_close(m_db);
        m_db = nullptr;
        throw;
    }
}

Database::~Database() {
    if (m_db != nullptr) {
        sqlite3_close(m_db);
    }
}

void Database::Exec(const std::string_view sql) const {
    std::string sql_text(sql);
    char *error_message = nullptr;
    const int result = sqlite3_exec(m_db, sql_text.c_str(), nullptr, nullptr, &error_message);
    if (result != SQLITE_OK) {
        std::string message = "SQLite statement failed";
        if (error_message != nullptr) {
            message += ": ";
            message += error_message;
            sqlite3_free(error_message);
        } else {
            message += ": ";
            message += sqlite3_errmsg(m_db);
        }
        throw std::runtime_error(message);
    }
    if (error_message != nullptr) {
        sqlite3_free(error_message);
    }
}

std::optional<std::wstring> Database::GetSetting(std::wstring_view key) const {
    Statement statement(
        m_db,
        "SELECT value FROM app_state WHERE key = ?1 LIMIT 1;"
    );
    BindText16(m_db, statement.get(), 1, key);
    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return std::nullopt;
    }
    if (result != SQLITE_ROW) {
        throw MakeSqliteError(m_db, "Unable to read setting");
    }
    return ColumnText16(statement.get(), 0);
}

void Database::SetSetting(std::wstring_view key, std::wstring_view value) const {
    Statement statement(
        m_db,
        "INSERT INTO app_state(key, value) VALUES(?1, ?2) "
        "ON CONFLICT(key) DO UPDATE SET value = excluded.value;"
    );
    BindText16(m_db, statement.get(), 1, key);
    BindText16(m_db, statement.get(), 2, value);
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to write setting");
    }
}

const char *Database::ListTable(DatabaseList list) const {
    switch (list) {
    case DatabaseList::IgnoredApplications:
        return "ignored_apps";
    case DatabaseList::IgnoredFormats:
        return "ignored_formats";
    case DatabaseList::IgnoredRegexps:
        return "ignored_regexps";
    }
    return "ignored_apps";
}

std::vector<std::wstring> Database::GetList(DatabaseList list) const {
    const std::string sql =
        std::string("SELECT value FROM ") + ListTable(list) + " ORDER BY rowid ASC;";
    Statement statement(m_db, sql.c_str());
    std::vector<std::wstring> values;
    while (true) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to read settings list");
        }
        values.push_back(ColumnText16(statement.get(), 0));
    }
    return values;
}

void Database::ReplaceList(DatabaseList list, const std::vector<std::wstring> &values) const {
    const std::string delete_sql = "DELETE FROM " + std::string(ListTable(list)) + ";";
    const std::string insert_sql =
        "INSERT OR IGNORE INTO " + std::string(ListTable(list)) + "(value) VALUES(?1);";

    Exec("BEGIN IMMEDIATE;");
    try {
        Exec(delete_sql);
        Statement statement(m_db, insert_sql.c_str());
        for (const std::wstring &value : values) {
            if (value.empty()) {
                continue;
            }
            sqlite3_reset(statement.get());
            sqlite3_clear_bindings(statement.get());
            BindText16(m_db, statement.get(), 1, value);
            if (sqlite3_step(statement.get()) != SQLITE_DONE) {
                throw MakeSqliteError(m_db, "Unable to write settings list");
            }
        }
        Exec("COMMIT;");
    } catch (...) {
        try {
            Exec("ROLLBACK;");
        } catch (...) {
        }
        throw;
    }
}

void Database::ResetIgnoredFormats() const {
    ReplaceList(DatabaseList::IgnoredFormats, DefaultIgnoredFormats());
}

void Database::SaveClipboard(const ClipboardCapture &capture, int max_unpinned) const {
    if (capture.fingerprint.empty() || capture.data.empty()) {
        return;
    }

    const int bounded_size = std::clamp(max_unpinned, 1, 999);
    const std::wstring stored_preview = StoredPreview(capture.preview);
    const sqlite3_int64 now = CurrentUnixMilliseconds();
    Exec("BEGIN IMMEDIATE;");
    try {
        sqlite3_int64 existing_id = 0;
        bool existing = false;
        {
            Statement find(m_db, "SELECT id FROM history_items WHERE fingerprint = ?1 LIMIT 1;");
            BindText16(m_db, find.get(), 1, capture.fingerprint);
            if (sqlite3_step(find.get()) == SQLITE_ROW) {
                existing_id = sqlite3_column_int64(find.get(), 0);
                existing = true;
            }
        }

        if (existing) {
            Statement update(
                m_db,
                "UPDATE history_items SET "
                "title = CASE WHEN title_custom = 0 THEN ?1 ELSE title END, "
                "content = ?2, application = ?3, copied_at = ?4, "
                "copy_count = copy_count + 1, has_text = ?5, has_image = ?6, has_files = ?7 "
                "WHERE id = ?8;"
            );
            BindText16(m_db, update.get(), 1, capture.title);
            BindText16(m_db, update.get(), 2, stored_preview);
            BindText16(m_db, update.get(), 3, capture.application);
            CheckSqliteResult(m_db, sqlite3_bind_int64(update.get(), 4, now), "Unable to bind copy time");
            CheckSqliteResult(m_db, sqlite3_bind_int(update.get(), 5, capture.has_text ? 1 : 0), "Unable to bind text flag");
            CheckSqliteResult(m_db, sqlite3_bind_int(update.get(), 6, capture.has_image ? 1 : 0), "Unable to bind image flag");
            CheckSqliteResult(m_db, sqlite3_bind_int(update.get(), 7, capture.has_files ? 1 : 0), "Unable to bind file flag");
            CheckSqliteResult(m_db, sqlite3_bind_int64(update.get(), 8, existing_id), "Unable to bind item id");
            if (sqlite3_step(update.get()) != SQLITE_DONE) {
                throw MakeSqliteError(m_db, "Unable to update clipboard history");
            }
        } else {
            Statement insert(
                m_db,
                "INSERT INTO history_items("
                "fingerprint, title, content, application, first_copied_at, copied_at, "
                "copy_count, has_text, has_image, has_files"
                ") VALUES(?1, ?2, ?3, ?4, ?5, ?5, 1, ?6, ?7, ?8);"
            );
            BindText16(m_db, insert.get(), 1, capture.fingerprint);
            BindText16(m_db, insert.get(), 2, capture.title);
            BindText16(m_db, insert.get(), 3, stored_preview);
            BindText16(m_db, insert.get(), 4, capture.application);
            CheckSqliteResult(m_db, sqlite3_bind_int64(insert.get(), 5, now), "Unable to bind copy time");
            CheckSqliteResult(m_db, sqlite3_bind_int(insert.get(), 6, capture.has_text ? 1 : 0), "Unable to bind text flag");
            CheckSqliteResult(m_db, sqlite3_bind_int(insert.get(), 7, capture.has_image ? 1 : 0), "Unable to bind image flag");
            CheckSqliteResult(m_db, sqlite3_bind_int(insert.get(), 8, capture.has_files ? 1 : 0), "Unable to bind file flag");
            if (sqlite3_step(insert.get()) != SQLITE_DONE) {
                throw MakeSqliteError(m_db, "Unable to insert clipboard history");
            }
            existing_id = sqlite3_last_insert_rowid(m_db);
        }

        Statement delete_data(m_db, "DELETE FROM history_data WHERE item_id = ?1;");
        CheckSqliteResult(m_db, sqlite3_bind_int64(delete_data.get(), 1, existing_id), "Unable to bind item id");
        if (sqlite3_step(delete_data.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to replace clipboard data");
        }

        Statement insert_data(
            m_db,
            "INSERT INTO history_data(item_id, format_name, format_id, data) "
            "VALUES(?1, ?2, ?3, ?4);"
        );
        for (const ClipboardFormatData &data : capture.data) {
            sqlite3_reset(insert_data.get());
            sqlite3_clear_bindings(insert_data.get());
            CheckSqliteResult(m_db, sqlite3_bind_int64(insert_data.get(), 1, existing_id), "Unable to bind item id");
            BindText16(m_db, insert_data.get(), 2, data.name);
            CheckSqliteResult(m_db, sqlite3_bind_int(insert_data.get(), 3, static_cast<int>(data.format)), "Unable to bind format id");
            BindBlob(m_db, insert_data.get(), 4, data.bytes);
            if (sqlite3_step(insert_data.get()) != SQLITE_DONE) {
                throw MakeSqliteError(m_db, "Unable to save clipboard data");
            }
        }

        TrimUnpinned(bounded_size);

        Exec("COMMIT;");
    } catch (...) {
        try {
            Exec("ROLLBACK;");
        } catch (...) {
        }
        throw;
    }
}

void Database::TrimUnpinned(int max_unpinned) const {
    const int bounded_size = std::clamp(max_unpinned, 1, 999);
    Statement trim(
        m_db,
        "DELETE FROM history_items WHERE pinned = 0 AND id NOT IN ("
        "SELECT id FROM history_items WHERE pinned = 0 "
        "ORDER BY copied_at DESC, id DESC LIMIT ?1);"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int(trim.get(), 1, bounded_size), "Unable to bind history size");
    if (sqlite3_step(trim.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to trim clipboard history");
    }
}

std::vector<ClipboardItem> Database::SearchHistory(
    std::wstring_view query,
    int search_mode,
    int sort_by,
    bool pins_at_bottom
) const {
    Statement statement(
        m_db,
        "SELECT id, title, content, application, COALESCE(pin, ''), pinned, "
        "first_copied_at, copied_at, copy_count, has_text, has_image, has_files "
        "FROM history_items ORDER BY id DESC LIMIT 5000;"
    );

    std::vector<ClipboardItem> all;
    while (true) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to query clipboard history");
        }

        ClipboardItem item;
        item.id = sqlite3_column_int64(statement.get(), 0);
        item.title = ColumnText16(statement.get(), 1);
        item.content = ColumnText16(statement.get(), 2);
        item.application = ColumnText16(statement.get(), 3);
        item.pin = ColumnText16(statement.get(), 4);
        item.pinned = sqlite3_column_int(statement.get(), 5) != 0;
        item.first_copied_at = sqlite3_column_int64(statement.get(), 6);
        item.copied_at = sqlite3_column_int64(statement.get(), 7);
        item.copy_count = sqlite3_column_int(statement.get(), 8);
        item.has_text = sqlite3_column_int(statement.get(), 9) != 0;
        item.has_image = sqlite3_column_int(statement.get(), 10) != 0;
        item.has_files = sqlite3_column_int(statement.get(), 11) != 0;
        all.push_back(std::move(item));
    }

    auto searchable = [](const ClipboardItem &item) -> std::wstring {
        if (!item.title.empty()) {
            return item.title;
        }
        return item.content;
    };

    std::vector<ClipboardItem> filtered;
    filtered.reserve(all.size());
    if (query.empty()) {
        filtered = std::move(all);
    } else if (search_mode == 0) {
        for (auto &item : all) {
            if (ContainsExact(searchable(item), query)) {
                filtered.push_back(std::move(item));
            }
        }
    } else if (search_mode == 1) {
        std::vector<std::pair<double, ClipboardItem>> fuzzy;
        for (auto &item : all) {
            if (const auto score = FuzzyScore(searchable(item), query)) {
                fuzzy.emplace_back(*score, std::move(item));
            }
        }
        std::stable_sort(fuzzy.begin(), fuzzy.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.first < rhs.first;
        });
        for (auto &entry : fuzzy) {
            filtered.push_back(std::move(entry.second));
        }
    } else if (search_mode == 2) {
        for (auto &item : all) {
            if (MatchRegex(searchable(item), query)) {
                filtered.push_back(std::move(item));
            }
        }
    } else {
        for (auto &item : all) {
            if (ContainsExact(searchable(item), query)) {
                filtered.push_back(std::move(item));
            }
        }
        if (filtered.empty()) {
            for (auto &item : all) {
                if (MatchRegex(searchable(item), query)) {
                    filtered.push_back(std::move(item));
                }
            }
        }
        if (filtered.empty()) {
            std::vector<std::pair<double, ClipboardItem>> fuzzy;
            for (auto &item : all) {
                if (const auto score = FuzzyScore(searchable(item), query)) {
                    fuzzy.emplace_back(*score, std::move(item));
                }
            }
            std::stable_sort(fuzzy.begin(), fuzzy.end(), [](const auto &lhs, const auto &rhs) {
                return lhs.first < rhs.first;
            });
            for (auto &entry : fuzzy) {
                filtered.push_back(std::move(entry.second));
            }
        }
    }

    std::stable_sort(filtered.begin(), filtered.end(), [sort_by, pins_at_bottom](const auto &lhs, const auto &rhs) {
        if (lhs.pinned != rhs.pinned) {
            return pins_at_bottom ? !lhs.pinned && rhs.pinned : lhs.pinned && !rhs.pinned;
        }
        if (sort_by == 1 && lhs.first_copied_at != rhs.first_copied_at) {
            return lhs.first_copied_at > rhs.first_copied_at;
        }
        if (sort_by == 2 && lhs.copy_count != rhs.copy_count) {
            return lhs.copy_count > rhs.copy_count;
        }
        if (lhs.copied_at != rhs.copied_at) {
            return lhs.copied_at > rhs.copied_at;
        }
        return lhs.id > rhs.id;
    });
    return filtered;
}

std::optional<ClipboardItem> Database::GetItem(sqlite3_int64 id, bool load_data) const {
    Statement statement(
        m_db,
        "SELECT id, title, content, application, COALESCE(pin, ''), pinned, "
        "first_copied_at, copied_at, copy_count, has_text, has_image, has_files "
        "FROM history_items WHERE id = ?1 LIMIT 1;"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, id), "Unable to bind item id");
    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        return std::nullopt;
    }

    ClipboardItem item;
    item.id = sqlite3_column_int64(statement.get(), 0);
    item.title = ColumnText16(statement.get(), 1);
    item.content = ColumnText16(statement.get(), 2);
    item.application = ColumnText16(statement.get(), 3);
    item.pin = ColumnText16(statement.get(), 4);
    item.pinned = sqlite3_column_int(statement.get(), 5) != 0;
    item.first_copied_at = sqlite3_column_int64(statement.get(), 6);
    item.copied_at = sqlite3_column_int64(statement.get(), 7);
    item.copy_count = sqlite3_column_int(statement.get(), 8);
    item.has_text = sqlite3_column_int(statement.get(), 9) != 0;
    item.has_image = sqlite3_column_int(statement.get(), 10) != 0;
    item.has_files = sqlite3_column_int(statement.get(), 11) != 0;
    if (load_data) {
        LoadData(item);
    }
    return item;
}

std::vector<ClipboardItem> Database::GetPinnedItems() const {
    std::vector<ClipboardItem> pinned;
    for (ClipboardItem &metadata : SearchHistory({}, 0, 0, false)) {
        if (!metadata.pinned) {
            continue;
        }
        if (auto item = GetItem(metadata.id, true)) {
            pinned.push_back(std::move(*item));
        }
    }
    return pinned;
}

void Database::LoadData(ClipboardItem &item) const {
    Statement statement(
        m_db,
        "SELECT format_name, format_id, data FROM history_data "
        "WHERE item_id = ?1 ORDER BY rowid ASC;"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, item.id), "Unable to bind item id");
    while (true) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to load clipboard data");
        }

        ClipboardFormatData data;
        data.name = ColumnText16(statement.get(), 0);
        data.format = static_cast<UINT>(sqlite3_column_int(statement.get(), 1));
        const auto *blob = static_cast<const unsigned char *>(sqlite3_column_blob(statement.get(), 2));
        const int bytes = sqlite3_column_bytes(statement.get(), 2);
        if (blob != nullptr && bytes > 0) {
            data.bytes.assign(blob, blob + bytes);
        }
        item.data.push_back(std::move(data));
    }
}

void Database::DeleteItem(sqlite3_int64 id) const {
    Statement statement(m_db, "DELETE FROM history_items WHERE id = ?1;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, id), "Unable to bind item id");
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to delete history item");
    }
}

void Database::DeleteUnpinned() const {
    Exec("DELETE FROM history_items WHERE pinned = 0;");
}

void Database::DeleteAll() const {
    Exec("DELETE FROM history_items;");
}

void Database::TogglePin(sqlite3_int64 id, std::wstring_view pin_key, bool pinned) const {
    Statement statement(
        m_db,
        "UPDATE history_items SET pinned = ?1, pin = ?2 WHERE id = ?3;"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int(statement.get(), 1, pinned ? 1 : 0), "Unable to bind pin flag");
    if (pinned) {
        BindText16(m_db, statement.get(), 2, pin_key);
    } else {
        CheckSqliteResult(m_db, sqlite3_bind_null(statement.get(), 2), "Unable to clear pin key");
    }
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 3, id), "Unable to bind item id");
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to update pin");
    }
}

void Database::UpdatePinnedItem(
    sqlite3_int64 id,
    std::wstring_view pin_key,
    std::wstring_view title,
    std::wstring_view content
) const {
    const std::wstring preview = MakeTitle(std::wstring(content), true);
    const std::vector<unsigned char> bytes(
        reinterpret_cast<const unsigned char *>(content.data()),
        reinterpret_cast<const unsigned char *>(content.data()) + (content.size() + 1) * sizeof(wchar_t)
    );

    Exec("BEGIN IMMEDIATE;");
    try {
        Statement update(
            m_db,
            "UPDATE history_items SET pin = ?1, pinned = 1, title = ?2, content = ?3, "
            "title_custom = 1, has_text = 1, has_image = 0, has_files = 0 "
            "WHERE id = ?4;"
        );
        BindText16(m_db, update.get(), 1, pin_key);
        BindText16(m_db, update.get(), 2, title.empty() ? preview : title);
        BindText16(m_db, update.get(), 3, preview);
        CheckSqliteResult(m_db, sqlite3_bind_int64(update.get(), 4, id), "Unable to bind item id");
        if (sqlite3_step(update.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to update pinned item");
        }

        Statement delete_data(m_db, "DELETE FROM history_data WHERE item_id = ?1;");
        CheckSqliteResult(m_db, sqlite3_bind_int64(delete_data.get(), 1, id), "Unable to bind item id");
        if (sqlite3_step(delete_data.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to replace pinned data");
        }

        Statement insert_data(
            m_db,
            "INSERT INTO history_data(item_id, format_name, format_id, data) VALUES(?1, ?2, ?3, ?4);"
        );
        CheckSqliteResult(m_db, sqlite3_bind_int64(insert_data.get(), 1, id), "Unable to bind item id");
        BindText16(m_db, insert_data.get(), 2, L"CF_UNICODETEXT");
        CheckSqliteResult(m_db, sqlite3_bind_int(insert_data.get(), 3, CF_UNICODETEXT), "Unable to bind text format");
        BindBlob(m_db, insert_data.get(), 4, bytes);
        if (sqlite3_step(insert_data.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to save pinned text");
        }
        Exec("COMMIT;");
    } catch (...) {
        try {
            Exec("ROLLBACK;");
        } catch (...) {
        }
        throw;
    }
}

void Database::UpdatePinnedMetadata(
    sqlite3_int64 id,
    std::wstring_view pin_key,
    std::wstring_view title
) const {
    Statement update(
        m_db,
        "UPDATE history_items SET pin = ?1, pinned = 1, title = ?2, title_custom = 1 "
        "WHERE id = ?3;"
    );
    BindText16(m_db, update.get(), 1, pin_key);
    BindText16(m_db, update.get(), 2, title);
    CheckSqliteResult(m_db, sqlite3_bind_int64(update.get(), 3, id), "Unable to bind item id");
    if (sqlite3_step(update.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to update pinned metadata");
    }
}

void Database::RegenerateTitles(bool show_special_symbols) const {
    Statement select(m_db, "SELECT id, content FROM history_items WHERE title_custom = 0;");
    std::vector<std::pair<sqlite3_int64, std::wstring>> values;
    while (sqlite3_step(select.get()) == SQLITE_ROW) {
        values.emplace_back(
            sqlite3_column_int64(select.get(), 0),
            MakeTitle(ColumnText16(select.get(), 1), show_special_symbols)
        );
    }

    Statement update(m_db, "UPDATE history_items SET title = ?1 WHERE id = ?2;");
    for (const auto &[id, title] : values) {
        sqlite3_reset(update.get());
        sqlite3_clear_bindings(update.get());
        BindText16(m_db, update.get(), 1, title);
        CheckSqliteResult(m_db, sqlite3_bind_int64(update.get(), 2, id), "Unable to bind item id");
        if (sqlite3_step(update.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to regenerate history title");
        }
    }
}

sqlite3_int64 Database::CountRows(const char *table) const {
    const std::string sql = "SELECT COUNT(*) FROM " + std::string(table) + ";";
    Statement statement(m_db, sql.c_str());
    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        throw MakeSqliteError(m_db, "Unable to count SQLite rows");
    }
    return sqlite3_column_int64(statement.get(), 0);
}

sqlite3_int64 Database::CountItems() const {
    return CountRows("history_items");
}

std::uintmax_t Database::StorageBytes() const {
    std::uintmax_t total = 0;
    const std::filesystem::path paths[] = {m_path, m_path.wstring() + L"-wal", m_path.wstring() + L"-shm"};
    for (const auto &path : paths) {
        std::error_code error;
        if (std::filesystem::is_regular_file(path, error)) {
            total += std::filesystem::file_size(path, error);
        }
    }
    return total;
}

void Database::MigrateLegacyHistory() const {
    if (GetSetting(L"schema.history_v2") == L"1") {
        return;
    }

    if (TableExists(m_db, "clipboard_history")) {
        Exec("BEGIN IMMEDIATE;");
        try {
            Statement select(m_db, "SELECT id, content, copied_at FROM clipboard_history ORDER BY copied_at ASC, id ASC;");
            Statement insert_item(
                m_db,
                "INSERT OR IGNORE INTO history_items("
                "fingerprint, title, content, first_copied_at, copied_at, copy_count, has_text"
                ") VALUES(?1, ?2, ?3, ?4, ?4, 1, 1);"
            );
            Statement insert_data(
                m_db,
                "INSERT OR IGNORE INTO history_data(item_id, format_name, format_id, data) VALUES(?1, ?2, ?3, ?4);"
            );
            while (sqlite3_step(select.get()) == SQLITE_ROW) {
                const sqlite3_int64 old_id = sqlite3_column_int64(select.get(), 0);
                const std::wstring content = ColumnText16(select.get(), 1);
                const sqlite3_int64 copied_at = sqlite3_column_int64(select.get(), 2);
                const std::wstring fingerprint = L"legacy:" + std::to_wstring(old_id);
                const std::wstring title = MakeTitle(content, true);
                const std::wstring stored_content = StoredPreview(content);

                sqlite3_reset(insert_item.get());
                sqlite3_clear_bindings(insert_item.get());
                BindText16(m_db, insert_item.get(), 1, fingerprint);
                BindText16(m_db, insert_item.get(), 2, title);
                BindText16(m_db, insert_item.get(), 3, stored_content);
                CheckSqliteResult(m_db, sqlite3_bind_int64(insert_item.get(), 4, copied_at), "Unable to bind legacy time");
                if (sqlite3_step(insert_item.get()) != SQLITE_DONE) {
                    throw MakeSqliteError(m_db, "Unable to migrate legacy history");
                }
                const sqlite3_int64 new_id = sqlite3_last_insert_rowid(m_db);

                std::vector<unsigned char> bytes(
                    reinterpret_cast<const unsigned char *>(content.data()),
                    reinterpret_cast<const unsigned char *>(content.data()) + (content.size() + 1) * sizeof(wchar_t)
                );
                sqlite3_reset(insert_data.get());
                sqlite3_clear_bindings(insert_data.get());
                CheckSqliteResult(m_db, sqlite3_bind_int64(insert_data.get(), 1, new_id), "Unable to bind item id");
                BindText16(m_db, insert_data.get(), 2, L"CF_UNICODETEXT");
                CheckSqliteResult(m_db, sqlite3_bind_int(insert_data.get(), 3, CF_UNICODETEXT), "Unable to bind text format");
                BindBlob(m_db, insert_data.get(), 4, bytes);
                if (sqlite3_step(insert_data.get()) != SQLITE_DONE) {
                    throw MakeSqliteError(m_db, "Unable to migrate legacy clipboard data");
                }
            }
            Exec("COMMIT;");
        } catch (...) {
            try {
                Exec("ROLLBACK;");
            } catch (...) {
            }
            throw;
        }
    }
    SetSetting(L"schema.history_v2", L"1");
}
