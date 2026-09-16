#include "Database.h"

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <cstring>
#include <fstream>
#include <limits>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

namespace {

constexpr std::uint32_t kPayloadMagic = 0x4D435059; // MCPY
constexpr std::uint32_t kPayloadVersion = 1;
constexpr std::uint32_t kMaximumPayloadRecords = 4096;
constexpr std::uint32_t kMaximumFormatNameBytes = 1024 * 1024;
constexpr std::uint64_t kMaximumPayloadBytes = 256ULL * 1024ULL * 1024ULL;

template <typename T>
void WritePayloadValue(std::ofstream& stream, T value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    if (!stream) {
        throw std::runtime_error("Unable to write clipboard payload");
    }
}

template <typename T>
T ReadPayloadValue(std::ifstream& stream) {
    T value{};
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!stream) {
        throw std::runtime_error("Unable to read clipboard payload");
    }
    return value;
}

void WritePayloadBytes(std::ofstream& stream, const void* bytes, std::size_t size) {
    if (size == 0) {
        return;
    }
    stream.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(size));
    if (!stream) {
        throw std::runtime_error("Unable to write clipboard payload");
    }
}

void ReadPayloadBytes(std::ifstream& stream, void* bytes, std::size_t size) {
    if (size == 0) {
        return;
    }
    stream.read(static_cast<char*>(bytes), static_cast<std::streamsize>(size));
    if (!stream) {
        throw std::runtime_error("Unable to read clipboard payload");
    }
}

std::uint32_t PayloadNameByteCount(const std::wstring& name) {
    if (name.size() > std::numeric_limits<std::uint32_t>::max() / sizeof(wchar_t)) {
        throw std::runtime_error("Clipboard format name is too large");
    }
    return static_cast<std::uint32_t>(name.size() * sizeof(wchar_t));
}

std::filesystem::path TemporaryPayloadPath(const std::filesystem::path& path) {
    return path.wstring() + L".tmp";
}

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

class Database::PayloadStore {
public:
    explicit PayloadStore(std::filesystem::path root);

    void Save(sqlite3_int64 item_id, const std::vector<ClipboardFormatData>& data) const;
    std::vector<ClipboardFormatData> Load(sqlite3_int64 item_id) const;
    void Remove(sqlite3_int64 item_id) const;
    std::uintmax_t StorageBytes() const;

private:
    std::filesystem::path PathFor(sqlite3_int64 item_id) const;

    std::filesystem::path m_root;
};

Database::PayloadStore::PayloadStore(std::filesystem::path root)
    : m_root(std::move(root)) {
    std::error_code error;
    std::filesystem::create_directories(m_root, error);
    if (error) {
        throw std::system_error(error, "Unable to create clipboard payload directory");
    }
}

std::filesystem::path Database::PayloadStore::PathFor(sqlite3_int64 item_id) const {
    if (item_id <= 0) {
        throw std::runtime_error("Invalid clipboard item id");
    }
    return m_root / (std::to_wstring(item_id) + L".payload");
}

void Database::PayloadStore::Save(
    sqlite3_int64 item_id,
    const std::vector<ClipboardFormatData>& data
) const {
    if (data.size() > kMaximumPayloadRecords) {
        throw std::runtime_error("Too many clipboard formats");
    }

    std::uint64_t total_bytes = 0;
    for (const ClipboardFormatData& item : data) {
        const std::uint32_t name_bytes = PayloadNameByteCount(item.name);
        if (name_bytes > kMaximumFormatNameBytes ||
            item.bytes.size() > kMaximumPayloadBytes ||
            total_bytes > kMaximumPayloadBytes - item.bytes.size()) {
            throw std::runtime_error("Clipboard payload is too large");
        }
        total_bytes += item.bytes.size();
    }

    const std::filesystem::path path = PathFor(item_id);
    const std::filesystem::path temporary = TemporaryPayloadPath(path);
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Unable to open clipboard payload file");
    }

    try {
        WritePayloadValue(stream, kPayloadMagic);
        WritePayloadValue(stream, kPayloadVersion);
        WritePayloadValue(stream, static_cast<std::uint32_t>(data.size()));
        for (const ClipboardFormatData& item : data) {
            const std::uint32_t name_bytes = PayloadNameByteCount(item.name);
            WritePayloadValue(stream, static_cast<std::uint32_t>(item.format));
            WritePayloadValue(stream, name_bytes);
            WritePayloadValue(stream, static_cast<std::uint64_t>(item.bytes.size()));
            WritePayloadBytes(stream, item.name.data(), name_bytes);
            WritePayloadBytes(stream, item.bytes.data(), item.bytes.size());
        }
        stream.flush();
        if (!stream) {
            throw std::runtime_error("Unable to flush clipboard payload file");
        }
        stream.close();

        if (!MoveFileExW(
            temporary.c_str(),
            path.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
        )) {
            throw std::system_error(
                static_cast<int>(GetLastError()),
                std::system_category(),
                "Unable to replace clipboard payload file"
            );
        }
    } catch (...) {
        stream.close();
        std::error_code error;
        std::filesystem::remove(temporary, error);
        throw;
    }
}

std::vector<ClipboardFormatData> Database::PayloadStore::Load(sqlite3_int64 item_id) const {
    const std::filesystem::path path = PathFor(item_id);
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open clipboard payload file");
    }

    const std::uint32_t magic = ReadPayloadValue<std::uint32_t>(stream);
    const std::uint32_t version = ReadPayloadValue<std::uint32_t>(stream);
    const std::uint32_t count = ReadPayloadValue<std::uint32_t>(stream);
    if (magic != kPayloadMagic || version != kPayloadVersion || count > kMaximumPayloadRecords) {
        throw std::runtime_error("Invalid clipboard payload file");
    }

    std::vector<ClipboardFormatData> data;
    data.reserve(count);
    std::uint64_t total_bytes = 0;
    for (std::uint32_t index = 0; index < count; ++index) {
        ClipboardFormatData item;
        item.format = static_cast<UINT>(ReadPayloadValue<std::uint32_t>(stream));
        const std::uint32_t name_bytes = ReadPayloadValue<std::uint32_t>(stream);
        const std::uint64_t payload_bytes = ReadPayloadValue<std::uint64_t>(stream);
        if (name_bytes > kMaximumFormatNameBytes ||
            name_bytes % sizeof(wchar_t) != 0 ||
            payload_bytes > kMaximumPayloadBytes ||
            total_bytes > kMaximumPayloadBytes - payload_bytes ||
            payload_bytes > std::numeric_limits<std::size_t>::max() ||
            name_bytes > std::numeric_limits<std::size_t>::max()) {
            throw std::runtime_error("Invalid clipboard payload size");
        }

        item.name.resize(name_bytes / sizeof(wchar_t));
        item.bytes.resize(static_cast<std::size_t>(payload_bytes));
        ReadPayloadBytes(stream, item.name.data(), name_bytes);
        ReadPayloadBytes(stream, item.bytes.data(), item.bytes.size());
        total_bytes += payload_bytes;
        data.push_back(std::move(item));
    }
    return data;
}

void Database::PayloadStore::Remove(sqlite3_int64 item_id) const {
    const std::filesystem::path path = PathFor(item_id);
    std::error_code error;
    std::filesystem::remove(path, error);
    if (error) {
        throw std::system_error(error, "Unable to remove clipboard payload file");
    }
    std::filesystem::remove(TemporaryPayloadPath(path), error);
    if (error) {
        throw std::system_error(error, "Unable to remove temporary clipboard payload file");
    }
}

std::uintmax_t Database::PayloadStore::StorageBytes() const {
    std::uintmax_t total = 0;
    std::error_code error;
    for (std::filesystem::recursive_directory_iterator it(m_root, error), end; it != end; it.increment(error)) {
        if (error) {
            throw std::system_error(error, "Unable to inspect clipboard payload directory");
        }
        if (!it->is_regular_file(error)) {
            if (error) {
                throw std::system_error(error, "Unable to inspect clipboard payload file");
            }
            continue;
        }
        const std::uintmax_t size = it->file_size(error);
        if (error) {
            throw std::system_error(error, "Unable to inspect clipboard payload size");
        }
        if (total > std::numeric_limits<std::uintmax_t>::max() - size) {
            return std::numeric_limits<std::uintmax_t>::max();
        }
        total += size;
    }
    if (error) {
        throw std::system_error(error, "Unable to inspect clipboard payload directory");
    }
    return total;
}

Database::Transaction::Transaction(const Database& database)
    : m_database(&database) {
    m_database->Exec("BEGIN IMMEDIATE;");
}

Database::Transaction::~Transaction() {
    if (m_database != nullptr && !m_committed) {
        try {
            m_database->Exec("ROLLBACK;");
        } catch (...) {
        }
    }
}

Database::Transaction::Transaction(Transaction&& other) noexcept
    : m_database(other.m_database), m_committed(other.m_committed) {
    other.m_database = nullptr;
    other.m_committed = true;
}

Database::Transaction& Database::Transaction::operator=(Transaction&& other) noexcept {
    if (this != &other) {
        if (m_database != nullptr && !m_committed) {
            try {
                m_database->Exec("ROLLBACK;");
            } catch (...) {
            }
        }
        m_database = other.m_database;
        m_committed = other.m_committed;
        other.m_database = nullptr;
        other.m_committed = true;
    }
    return *this;
}

void Database::Transaction::Commit() {
    if (m_database == nullptr || m_committed) {
        return;
    }
    m_database->Exec("COMMIT;");
    m_committed = true;
}

Database::Database(const std::filesystem::path &path)
    : m_path(path),
      m_payloadStore(std::make_unique<PayloadStore>(
          path.parent_path() / (path.filename().wstring() + L".payloads")
      )) {
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
        CreateHistoryTables();
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

void Database::CreateHistoryTables() {
    Exec(
        "CREATE TABLE IF NOT EXISTS history_items ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "fingerprint TEXT NOT NULL UNIQUE,"
        "title TEXT NOT NULL DEFAULT '',"
        "preview TEXT NOT NULL DEFAULT '',"
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

    auto transaction = BeginTransaction();
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
    transaction.Commit();
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
    sqlite3_int64 item_id = 0;
    {
        auto transaction = BeginTransaction();
        Statement find(m_db, "SELECT id FROM history_items WHERE fingerprint = ?1 LIMIT 1;");
        BindText16(m_db, find.get(), 1, capture.fingerprint);
        const int result = sqlite3_step(find.get());
        const bool existing = result == SQLITE_ROW;
        if (result != SQLITE_ROW && result != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to find clipboard history");
        }
        if (existing) {
            item_id = sqlite3_column_int64(find.get(), 0);
            Statement update(
                m_db,
                "UPDATE history_items SET "
                "title = CASE WHEN title_custom = 0 THEN ?1 ELSE title END, "
                "preview = ?2, application = ?3, copied_at = ?4, "
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
            CheckSqliteResult(m_db, sqlite3_bind_int64(update.get(), 8, item_id), "Unable to bind item id");
            if (sqlite3_step(update.get()) != SQLITE_DONE) {
                throw MakeSqliteError(m_db, "Unable to update clipboard history");
            }
        } else {
            Statement insert(
                m_db,
                "INSERT INTO history_items("
                "fingerprint, title, preview, application, first_copied_at, copied_at, "
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
            item_id = sqlite3_last_insert_rowid(m_db);
        }

        m_payloadStore->Save(item_id, capture.data);
        transaction.Commit();
    }
    TrimUnpinned(bounded_size);
}

void Database::TrimUnpinned(int max_unpinned) const {
    const int bounded_size = std::clamp(max_unpinned, 1, 999);
    Statement select(
        m_db,
        "SELECT id FROM history_items WHERE pinned = 0 AND id NOT IN ("
        "SELECT id FROM history_items WHERE pinned = 0 "
        "ORDER BY copied_at DESC, id DESC LIMIT ?1);"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int(select.get(), 1, bounded_size), "Unable to bind history size");
    std::vector<sqlite3_int64> removed_ids;
    while (true) {
        const int result = sqlite3_step(select.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to find expired clipboard history");
        }
        removed_ids.push_back(sqlite3_column_int64(select.get(), 0));
    }
    if (removed_ids.empty()) {
        return;
    }

    auto transaction = BeginTransaction();
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
    transaction.Commit();
    for (const sqlite3_int64 id : removed_ids) {
        m_payloadStore->Remove(id);
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
        "SELECT id, title, preview, application, COALESCE(pin, ''), pinned, "
        "first_copied_at, copied_at, copy_count, has_text, has_image, has_files "
        "FROM history_items ORDER BY id DESC;"
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
        item.preview = ColumnText16(statement.get(), 2);
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
        return item.preview;
    };

    bool fuzzyResults = false;
    std::optional<std::wregex> regex;
    if (!query.empty() && (search_mode == 2 || search_mode == 3)) {
        try {
            regex.emplace(std::wstring(query));
        } catch (const std::regex_error&) {
            regex.reset();
        }
    }
    const auto matchesRegex = [&regex](std::wstring_view text) {
        if (!regex) {
            return false;
        }
        const std::wstring value(text);
        return std::regex_search(value, *regex);
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
        fuzzyResults = true;
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
            if (matchesRegex(searchable(item))) {
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
                if (matchesRegex(searchable(item))) {
                    filtered.push_back(std::move(item));
                }
            }
        }
        if (filtered.empty()) {
            fuzzyResults = true;
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

    std::stable_sort(filtered.begin(), filtered.end(), [sort_by, pins_at_bottom, fuzzyResults](const auto &lhs, const auto &rhs) {
        if (lhs.pinned != rhs.pinned) {
            return pins_at_bottom ? !lhs.pinned && rhs.pinned : lhs.pinned && !rhs.pinned;
        }
        if (fuzzyResults) return false; // stable_sort preserves relevance within each section.
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

std::optional<ClipboardItem> Database::GetItem(sqlite3_int64 id, bool load_payload) const {
    Statement statement(
        m_db,
        "SELECT id, title, preview, application, COALESCE(pin, ''), pinned, "
        "first_copied_at, copied_at, copy_count, has_text, has_image, has_files "
        "FROM history_items WHERE id = ?1 LIMIT 1;"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, id), "Unable to bind item id");
    const int result = sqlite3_step(statement.get());
    if (result == SQLITE_DONE) {
        return std::nullopt;
    }
    if (result != SQLITE_ROW) {
        throw MakeSqliteError(m_db, "Unable to read clipboard item");
    }

    ClipboardItem item;
    item.id = sqlite3_column_int64(statement.get(), 0);
    item.title = ColumnText16(statement.get(), 1);
    item.preview = ColumnText16(statement.get(), 2);
    item.application = ColumnText16(statement.get(), 3);
    item.pin = ColumnText16(statement.get(), 4);
    item.pinned = sqlite3_column_int(statement.get(), 5) != 0;
    item.first_copied_at = sqlite3_column_int64(statement.get(), 6);
    item.copied_at = sqlite3_column_int64(statement.get(), 7);
    item.copy_count = sqlite3_column_int(statement.get(), 8);
    item.has_text = sqlite3_column_int(statement.get(), 9) != 0;
    item.has_image = sqlite3_column_int(statement.get(), 10) != 0;
    item.has_files = sqlite3_column_int(statement.get(), 11) != 0;
    if (load_payload) {
        LoadPayload(item);
    }
    return item;
}

std::vector<ClipboardItem> Database::GetPinnedItems(bool load_payload) const {
    std::vector<ClipboardItem> pinned;
    for (ClipboardItem &metadata : SearchHistory({}, 0, 0, false)) {
        if (!metadata.pinned) {
            continue;
        }
        if (!load_payload) {
            pinned.push_back(std::move(metadata));
        } else if (auto item = GetItem(metadata.id, true)) {
            pinned.push_back(std::move(*item));
        }
    }
    return pinned;
}

void Database::LoadPayload(ClipboardItem &item) const {
    item.data = m_payloadStore->Load(item.id);
}

std::vector<sqlite3_int64> Database::SelectItemIds(std::string_view condition) const {
    std::string sql = "SELECT id FROM history_items";
    if (!condition.empty()) {
        sql += " WHERE ";
        sql += condition;
    }
    sql += ";";

    Statement statement(m_db, sql.c_str());
    std::vector<sqlite3_int64> ids;
    while (true) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to read clipboard item ids");
        }
        ids.push_back(sqlite3_column_int64(statement.get(), 0));
    }
    return ids;
}

void Database::RemovePayloads(const std::vector<sqlite3_int64>& ids) const {
    for (const sqlite3_int64 id : ids) {
        m_payloadStore->Remove(id);
    }
}

void Database::DeleteItem(sqlite3_int64 id) const {
    {
        auto transaction = BeginTransaction();
        Statement statement(m_db, "DELETE FROM history_items WHERE id = ?1;");
        CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, id), "Unable to bind item id");
        if (sqlite3_step(statement.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to delete history item");
        }
        transaction.Commit();
    }
    m_payloadStore->Remove(id);
}

void Database::DeleteUnpinned() const {
    const std::vector<sqlite3_int64> ids = SelectItemIds("pinned = 0");
    {
        auto transaction = BeginTransaction();
        Exec("DELETE FROM history_items WHERE pinned = 0;");
        transaction.Commit();
    }
    RemovePayloads(ids);
}

void Database::DeleteAll() const {
    const std::vector<sqlite3_int64> ids = SelectItemIds({});
    {
        auto transaction = BeginTransaction();
        Exec("DELETE FROM history_items;");
        transaction.Commit();
    }
    RemovePayloads(ids);
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
    std::wstring_view text
) const {
    const std::wstring preview = StoredPreview(MakeTitle(std::wstring(text), true));
    ClipboardFormatData data;
    data.name = L"CF_UNICODETEXT";
    data.format = CF_UNICODETEXT;
    data.bytes.resize((text.size() + 1) * sizeof(wchar_t));
    std::memcpy(data.bytes.data(), text.data(), text.size() * sizeof(wchar_t));
    std::memset(data.bytes.data() + text.size() * sizeof(wchar_t), 0, sizeof(wchar_t));

    {
        auto transaction = BeginTransaction();
        Statement update(
            m_db,
            "UPDATE history_items SET pin = ?1, pinned = 1, title = ?2, preview = ?3, "
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
        m_payloadStore->Save(id, {data});
        transaction.Commit();
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
    Statement select(m_db, "SELECT id, preview FROM history_items WHERE title_custom = 0;");
    std::vector<std::pair<sqlite3_int64, std::wstring>> values;
    while (true) {
        const int result = sqlite3_step(select.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to read history titles");
        }
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
    total += m_payloadStore->StorageBytes();
    return total;
}

void Database::MarkCopied(sqlite3_int64 id) const {
    Statement statement(m_db, "UPDATE history_items SET copied_at = ?1, copy_count = copy_count + 1 WHERE id = ?2;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, CurrentUnixMilliseconds()), "Unable to bind copy time");
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 2, id), "Unable to bind item id");
    if (sqlite3_step(statement.get()) != SQLITE_DONE) throw MakeSqliteError(m_db, "Unable to update copy count");
}
