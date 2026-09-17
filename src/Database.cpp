#include "Database.h"

#include <algorithm>
#include <chrono>
#include <cwctype>
#include <cstring>
#include <limits>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_set>
#include <utility>

#include <shellapi.h>
#include <shlobj.h>

namespace {

constexpr std::uint32_t kMaximumPayloadRecords = 4096;
constexpr std::uint32_t kMaximumFormatNameBytes = 1024 * 1024;
constexpr std::uint64_t kMaximumPayloadBytes = 256ULL * 1024ULL * 1024ULL;

struct SearchParts {
    std::wstring body;
    std::wstring paths;
};

struct SearchAccumulator {
    std::wstring unicode_text;
    std::wstring ansi_text;
    std::wstring rich_text;
    std::wstring paths;
};

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

bool EqualInsensitive(std::wstring_view lhs, std::wstring_view rhs) {
    return Lower(lhs) == Lower(rhs);
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
        L"CF_CLIPBOARD_VIEWER_IGNORE",
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

void BindBlob(
    sqlite3 *db,
    sqlite3_stmt *statement,
    int index,
    const std::vector<unsigned char> &bytes
) {
    CheckSqliteResult(
        db,
        sqlite3_bind_blob64(
            statement,
            index,
            bytes.data(),
            static_cast<sqlite3_uint64>(bytes.size()),
            SQLITE_TRANSIENT
        ),
        "Unable to bind SQLite blob"
    );
}

std::vector<unsigned char> ColumnBlob(sqlite3_stmt *statement, int column) {
    const int byte_count = sqlite3_column_bytes(statement, column);
    if (byte_count <= 0) {
        return {};
    }
    const auto *bytes = static_cast<const unsigned char *>(sqlite3_column_blob(statement, column));
    if (bytes == nullptr) {
        return {};
    }
    return {bytes, bytes + byte_count};
}

bool IsUnicodeTextFormat(const ClipboardFormatData &data) {
    return data.format == CF_UNICODETEXT || EqualInsensitive(data.name, L"CF_UNICODETEXT");
}

bool IsAnsiTextFormat(const ClipboardFormatData &data) {
    return data.format == CF_TEXT || EqualInsensitive(data.name, L"CF_TEXT");
}

bool IsRichTextFormat(const ClipboardFormatData &data) {
    return EqualInsensitive(data.name, L"HTML Format") ||
        EqualInsensitive(data.name, L"Rich Text Format");
}

bool IsFilesFormat(const ClipboardFormatData &data) {
    return data.format == CF_HDROP || EqualInsensitive(data.name, L"CF_HDROP");
}

std::wstring DecodeUnicodeText(const std::vector<unsigned char> &bytes) {
    if (bytes.size() < sizeof(wchar_t)) {
        return {};
    }

    const size_t count = bytes.size() / sizeof(wchar_t);
    std::wstring result(count, L'\0');
    std::memcpy(result.data(), bytes.data(), count * sizeof(wchar_t));
    const size_t nul = result.find(L'\0');
    if (nul != std::wstring::npos) {
        result.resize(nul);
    }
    return result;
}

std::wstring DecodeAnsiText(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return {};
    }

    const size_t nul = std::find(bytes.begin(), bytes.end(), 0) - bytes.begin();
    if (nul == 0 || nul > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const int source_length = static_cast<int>(nul);
    const auto *source = reinterpret_cast<const char *>(bytes.data());
    const int length = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        source,
        source_length,
        nullptr,
        0
    );
    if (length <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        source,
        source_length,
        result.data(),
        length
    );
    return result;
}

std::wstring DecodeByteText(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return {};
    }

    const size_t nul = std::find(bytes.begin(), bytes.end(), 0) - bytes.begin();
    if (nul == 0 || nul > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const int source_length = static_cast<int>(nul);
    const auto *source = reinterpret_cast<const char *>(bytes.data());

    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int length = MultiByteToWideChar(
        code_page,
        flags,
        source,
        source_length,
        nullptr,
        0
    );
    if (length <= 0) {
        code_page = CP_ACP;
        flags = MB_PRECOMPOSED;
        length = MultiByteToWideChar(
            code_page,
            flags,
            source,
            source_length,
            nullptr,
            0
        );
    }
    if (length <= 0) {
        return {};
    }

    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(
        code_page,
        flags,
        source,
        source_length,
        result.data(),
        length
    );
    return result;
}

std::wstring StripHtml(std::wstring value) {
    const size_t header_end = value.find(L"\r\n\r\n");
    if (header_end != std::wstring::npos) {
        value.erase(0, header_end + 4);
    }

    std::wstring result;
    result.reserve(value.size());
    bool in_tag = false;
    for (size_t index = 0; index < value.size(); ++index) {
        const wchar_t character = value[index];
        if (character == L'<') {
            in_tag = true;
            result.push_back(L' ');
            continue;
        }
        if (in_tag) {
            if (character == L'>') {
                in_tag = false;
            }
            continue;
        }
        if (character == L'&') {
            const size_t end = value.find(L';', index + 1);
            if (end != std::wstring::npos && end - index <= 16) {
                const std::wstring_view entity(value.data() + index + 1, end - index - 1);
                if (entity == L"nbsp") result += L' ';
                else if (entity == L"amp") result += L'&';
                else if (entity == L"lt") result += L'<';
                else if (entity == L"gt") result += L'>';
                else if (entity == L"quot") result += L'\"';
                else result.append(value, index, end - index + 1);
                index = end;
                continue;
            }
        }
        result.push_back(character);
    }
    return TrimWhitespace(std::move(result));
}

std::wstring StripRtf(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (size_t index = 0; index < value.size();) {
        const wchar_t character = value[index++];
        if (character == L'{' || character == L'}') {
            continue;
        }
        if (character != L'\\') {
            result.push_back(character);
            continue;
        }

        if (index >= value.size()) {
            break;
        }
        if (value[index] == L'\'') {
            index = std::min(index + 3, value.size());
            continue;
        }
        const size_t word_begin = index;
        while (index < value.size() && std::iswalpha(value[index])) {
            ++index;
        }
        const std::wstring_view word(value.data() + word_begin, index - word_begin);
        if (word == L"par" || word == L"line") {
            result.push_back(L'\n');
        }
        if (index < value.size() && (value[index] == L'-' || std::iswdigit(value[index]))) {
            ++index;
            while (index < value.size() && std::iswdigit(value[index])) {
                ++index;
            }
        }
        if (index < value.size() && value[index] == L' ') {
            ++index;
        }
    }
    return TrimWhitespace(std::move(result));
}

std::wstring ExtractDropPaths(const std::vector<unsigned char> &bytes) {
    if (bytes.size() < sizeof(DROPFILES)) {
        return {};
    }

    DROPFILES header{};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.pFiles < sizeof(DROPFILES) || header.pFiles >= bytes.size()) {
        return {};
    }

    std::wstring result;
    size_t offset = header.pFiles;
    while (offset < bytes.size()) {
        std::wstring path;
        if (header.fWide) {
            while (offset + sizeof(wchar_t) <= bytes.size()) {
                wchar_t character = L'\0';
                std::memcpy(&character, bytes.data() + offset, sizeof(character));
                offset += sizeof(character);
                if (character == L'\0') {
                    break;
                }
                path.push_back(character);
            }
        } else {
            std::string ansi;
            while (offset < bytes.size() && bytes[offset] != 0) {
                ansi.push_back(static_cast<char>(bytes[offset++]));
            }
            if (offset < bytes.size()) {
                ++offset;
            }
            if (!ansi.empty()) {
                const int length = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    ansi.data(),
                    static_cast<int>(ansi.size()),
                    nullptr,
                    0
                );
                if (length > 0) {
                    path.resize(static_cast<size_t>(length));
                    MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        ansi.data(),
                        static_cast<int>(ansi.size()),
                        path.data(),
                        length
                    );
                }
            }
        }
        if (path.empty()) {
            break;
        }
        if (!result.empty()) {
            result += L"; ";
        }
        result += path;
    }
    return result;
}

void AccumulateSearchFormat(SearchAccumulator &accumulator, const ClipboardFormatData &format) {
    if (IsUnicodeTextFormat(format) && accumulator.unicode_text.empty()) {
        accumulator.unicode_text = DecodeUnicodeText(format.bytes);
    } else if (IsAnsiTextFormat(format) && accumulator.ansi_text.empty()) {
        accumulator.ansi_text = DecodeAnsiText(format.bytes);
    } else if (IsRichTextFormat(format) && accumulator.rich_text.empty()) {
        const std::wstring decoded = DecodeByteText(format.bytes);
        accumulator.rich_text = EqualInsensitive(format.name, L"HTML Format")
            ? StripHtml(decoded)
            : StripRtf(decoded);
    } else if (IsFilesFormat(format) && accumulator.paths.empty()) {
        accumulator.paths = ExtractDropPaths(format.bytes);
    }
}

SearchParts SearchPartsFromAccumulator(SearchAccumulator accumulator) {
    SearchParts result;
    result.body = !accumulator.unicode_text.empty()
        ? std::move(accumulator.unicode_text)
        : (!accumulator.ansi_text.empty()
            ? std::move(accumulator.ansi_text)
            : std::move(accumulator.rich_text));
    result.paths = std::move(accumulator.paths);
    return result;
}

SearchParts SearchPartsFromFormats(const std::vector<ClipboardFormatData> &data) {
    SearchAccumulator accumulator;
    for (const ClipboardFormatData &format : data) {
        AccumulateSearchFormat(accumulator, format);
    }
    return SearchPartsFromAccumulator(std::move(accumulator));
}

std::wstring MakeFtsPhrase(std::wstring_view query) {
    std::wstring result = L"\"";
    for (const wchar_t character : query) {
        result += character;
        if (character == L'\"') {
            result += L'\"';
        }
    }
    result += L"\"";
    return result;
}

} // namespace

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
    : m_path(path) {
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
    Exec(
        "CREATE TABLE IF NOT EXISTS clipboard_formats ("
        "item_id INTEGER NOT NULL REFERENCES history_items(id) ON DELETE CASCADE,"
        "sequence INTEGER NOT NULL,"
        "format INTEGER NOT NULL,"
        "name TEXT NOT NULL,"
        "data BLOB NOT NULL,"
        "PRIMARY KEY(item_id, sequence)"
        ");"
    );
    Exec(
        "CREATE TABLE IF NOT EXISTS history_search_documents ("
        "item_id INTEGER PRIMARY KEY REFERENCES history_items(id) ON DELETE CASCADE,"
        "body TEXT NOT NULL DEFAULT '',"
        "paths TEXT NOT NULL DEFAULT ''"
        ");"
    );
    Exec(
        "CREATE VIRTUAL TABLE IF NOT EXISTS history_fts USING fts5("
        "body, paths, content='', contentless_delete=1, "
        "tokenize='trigram'"
        ");"
    );

    bool has_missing_documents = false;
    {
        Statement missing(
            m_db,
            "SELECT 1 FROM history_items AS items "
            "LEFT JOIN history_search_documents AS documents ON documents.item_id = items.id "
            "WHERE documents.item_id IS NULL LIMIT 1;"
        );
        const int result = sqlite3_step(missing.get());
        if (result == SQLITE_ROW) {
            has_missing_documents = true;
        } else if (result != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to inspect search documents");
        }
    }
    if (has_missing_documents) {
        RebuildSearchDocuments();
    }
}

void Database::ReplaceFormats(
    sqlite3_int64 item_id,
    const std::vector<ClipboardFormatData> &data
) const {
    if (item_id <= 0) {
        throw std::runtime_error("Invalid clipboard item id");
    }
    if (data.size() > kMaximumPayloadRecords) {
        throw std::runtime_error("Too many clipboard formats");
    }

    std::uint64_t total_bytes = 0;
    for (const ClipboardFormatData &format : data) {
        if (format.name.size() > kMaximumFormatNameBytes / sizeof(wchar_t) ||
            format.bytes.size() > kMaximumPayloadBytes ||
            total_bytes > kMaximumPayloadBytes - format.bytes.size()) {
            throw std::runtime_error("Clipboard payload is too large");
        }
        total_bytes += format.bytes.size();
    }

    Statement remove(m_db, "DELETE FROM clipboard_formats WHERE item_id = ?1;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(remove.get(), 1, item_id), "Unable to bind item id");
    if (sqlite3_step(remove.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to replace clipboard formats");
    }

    Statement insert(
        m_db,
        "INSERT INTO clipboard_formats(item_id, sequence, format, name, data) "
        "VALUES(?1, ?2, ?3, ?4, ?5);"
    );
    for (size_t index = 0; index < data.size(); ++index) {
        sqlite3_reset(insert.get());
        sqlite3_clear_bindings(insert.get());
        CheckSqliteResult(m_db, sqlite3_bind_int64(insert.get(), 1, item_id), "Unable to bind item id");
        CheckSqliteResult(
            m_db,
            sqlite3_bind_int64(insert.get(), 2, static_cast<sqlite3_int64>(index)),
            "Unable to bind clipboard format sequence"
        );
        CheckSqliteResult(
            m_db,
            sqlite3_bind_int64(insert.get(), 3, static_cast<sqlite3_int64>(data[index].format)),
            "Unable to bind clipboard format id"
        );
        BindText16(m_db, insert.get(), 4, data[index].name);
        BindBlob(m_db, insert.get(), 5, data[index].bytes);
        if (sqlite3_step(insert.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to insert clipboard format");
        }
    }
}

void Database::ReplaceSearchIndex(
    sqlite3_int64 item_id,
    std::wstring_view body,
    std::wstring_view paths
) const {
    Statement remove_document(
        m_db,
        "DELETE FROM history_search_documents WHERE item_id = ?1;"
    );
    CheckSqliteResult(
        m_db,
        sqlite3_bind_int64(remove_document.get(), 1, item_id),
        "Unable to bind search document id"
    );
    if (sqlite3_step(remove_document.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to replace search document");
    }

    Statement insert_document(
        m_db,
        "INSERT INTO history_search_documents(item_id, body, paths) VALUES(?1, ?2, ?3);"
    );
    CheckSqliteResult(
        m_db,
        sqlite3_bind_int64(insert_document.get(), 1, item_id),
        "Unable to bind search document id"
    );
    BindText16(m_db, insert_document.get(), 2, body);
    BindText16(m_db, insert_document.get(), 3, paths);
    if (sqlite3_step(insert_document.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to insert search document");
    }

    Statement remove(m_db, "DELETE FROM history_fts WHERE rowid = ?1;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(remove.get(), 1, item_id), "Unable to bind search item id");
    if (sqlite3_step(remove.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to replace clipboard search index");
    }
    if (body.empty() && paths.empty()) {
        return;
    }

    Statement insert(
        m_db,
        "INSERT INTO history_fts(rowid, body, paths) VALUES(?1, ?2, ?3);"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int64(insert.get(), 1, item_id), "Unable to bind search item id");
    BindText16(m_db, insert.get(), 2, body);
    BindText16(m_db, insert.get(), 3, paths);
    if (sqlite3_step(insert.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to insert clipboard search index");
    }
}

std::vector<sqlite3_int64> Database::SearchIndexIds(
    std::wstring_view query,
    const SearchCancellation &is_cancelled
) const {
    if (query.size() < 3) {
        return {};
    }

    Statement statement(
        m_db,
        "SELECT rowid FROM history_fts WHERE history_fts MATCH ?1;"
    );
    const std::wstring phrase = MakeFtsPhrase(query);
    BindText16(m_db, statement.get(), 1, phrase);
    std::vector<sqlite3_int64> ids;
    while (true) {
        if (is_cancelled && is_cancelled()) {
            return {};
        }
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to search clipboard text");
        }
        ids.push_back(sqlite3_column_int64(statement.get(), 0));
    }
    return ids;
}

void Database::ForEachRawSearchDocument(
    const std::function<void(const SearchDocument &)> &callback,
    const SearchCancellation &is_cancelled
) const {
    Statement statement(
        m_db,
        "SELECT item_id, format, name, data FROM clipboard_formats "
        "WHERE format IN (1, 13, 15) "
        "OR name COLLATE NOCASE IN ("
        "'CF_TEXT', 'CF_UNICODETEXT', 'CF_HDROP', 'HTML Format', 'Rich Text Format'"
        ") ORDER BY item_id DESC, sequence ASC;"
    );

    bool has_item = false;
    sqlite3_int64 item_id = 0;
    SearchAccumulator accumulator;
    const auto cancelled = [&]() {
        return is_cancelled && is_cancelled();
    };
    const auto emit = [&]() {
        if (!has_item) {
            return;
        }
        SearchParts parts = SearchPartsFromAccumulator(std::move(accumulator));
        accumulator = {};
        if (parts.body.empty() && parts.paths.empty()) {
            return;
        }
        callback(SearchDocument{
            item_id,
            std::move(parts.body),
            std::move(parts.paths)
        });
    };

    while (true) {
        if (cancelled()) {
            return;
        }
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            if (!cancelled()) {
                emit();
            }
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to load clipboard search text");
        }

        const sqlite3_int64 row_item_id = sqlite3_column_int64(statement.get(), 0);
        if (!has_item) {
            has_item = true;
            item_id = row_item_id;
        } else if (row_item_id != item_id) {
            emit();
            if (cancelled()) {
                return;
            }
            item_id = row_item_id;
        }

        ClipboardFormatData format;
        format.format = static_cast<UINT>(sqlite3_column_int64(statement.get(), 1));
        format.name = ColumnText16(statement.get(), 2);
        format.bytes = ColumnBlob(statement.get(), 3);
        AccumulateSearchFormat(accumulator, format);
    }
}

void Database::RebuildSearchDocuments() const {
    auto transaction = BeginTransaction();
    Exec("DELETE FROM history_search_documents;");

    Statement insert(
        m_db,
        "INSERT INTO history_search_documents(item_id, body, paths) VALUES(?1, ?2, ?3);"
    );
    ForEachRawSearchDocument([&](const SearchDocument &document) {
        sqlite3_reset(insert.get());
        sqlite3_clear_bindings(insert.get());
        CheckSqliteResult(
            m_db,
            sqlite3_bind_int64(insert.get(), 1, document.id),
            "Unable to bind search document id"
        );
        BindText16(m_db, insert.get(), 2, document.body);
        BindText16(m_db, insert.get(), 3, document.paths);
        if (sqlite3_step(insert.get()) != SQLITE_DONE) {
            throw MakeSqliteError(m_db, "Unable to rebuild search documents");
        }
    }, {});

    Exec(
        "INSERT OR IGNORE INTO history_search_documents(item_id, body, paths) "
        "SELECT id, '', '' FROM history_items;"
    );
    transaction.Commit();
}

void Database::ForEachSearchDocument(
    const std::function<void(const SearchDocument &)> &callback,
    const SearchCancellation &is_cancelled
) const {
    Statement statement(
        m_db,
        "SELECT item_id, body, paths FROM history_search_documents ORDER BY item_id DESC;"
    );
    while (true) {
        if (is_cancelled && is_cancelled()) {
            return;
        }
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            return;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to load clipboard search text");
        }
        SearchDocument document;
        document.id = sqlite3_column_int64(statement.get(), 0);
        document.body = ColumnText16(statement.get(), 1);
        document.paths = ColumnText16(statement.get(), 2);
        callback(document);
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
    const SearchParts search = SearchPartsFromFormats(capture.data);
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
        bool formats_present = false;
        if (existing) {
            item_id = sqlite3_column_int64(find.get(), 0);
            Statement format_check(
                m_db,
                "SELECT 1 FROM clipboard_formats WHERE item_id = ?1 LIMIT 1;"
            );
            CheckSqliteResult(
                m_db,
                sqlite3_bind_int64(format_check.get(), 1, item_id),
                "Unable to bind clipboard format item id"
            );
            const int format_result = sqlite3_step(format_check.get());
            if (format_result == SQLITE_ROW) {
                formats_present = true;
            } else if (format_result != SQLITE_DONE) {
                throw MakeSqliteError(m_db, "Unable to inspect clipboard formats");
            }
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

        if (!existing || !formats_present) {
            ReplaceFormats(item_id, capture.data);
            ReplaceSearchIndex(item_id, search.body, search.paths);
        }
        transaction.Commit();
    }
    TrimUnpinned(bounded_size);
}

void Database::TrimUnpinned(int max_unpinned) const {
    const int bounded_size = std::clamp(max_unpinned, 1, 999);
    std::vector<sqlite3_int64> removed_ids;
    {
        Statement select(
            m_db,
            "SELECT id FROM history_items WHERE pinned = 0 AND id NOT IN ("
            "SELECT id FROM history_items WHERE pinned = 0 "
            "ORDER BY copied_at DESC, id DESC LIMIT ?1);"
        );
        CheckSqliteResult(m_db, sqlite3_bind_int(select.get(), 1, bounded_size), "Unable to bind history size");
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
    }
    if (removed_ids.empty()) {
        return;
    }

    auto transaction = BeginTransaction();
    Statement remove_index(
        m_db,
        "DELETE FROM history_fts WHERE rowid IN ("
        "SELECT id FROM history_items WHERE pinned = 0 AND id NOT IN ("
        "SELECT id FROM history_items WHERE pinned = 0 "
        "ORDER BY copied_at DESC, id DESC LIMIT ?1));"
    );
    CheckSqliteResult(m_db, sqlite3_bind_int(remove_index.get(), 1, bounded_size), "Unable to bind history size");
    if (sqlite3_step(remove_index.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to trim clipboard search index");
    }
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
}

std::vector<ClipboardItem> Database::SearchHistory(
    std::wstring_view query,
    int search_mode,
    int sort_by,
    bool pins_at_bottom,
    SearchCancellation is_cancelled
) const {
    const auto cancelled = [&]() {
        return is_cancelled && is_cancelled();
    };
    if (cancelled()) {
        return {};
    }

    Statement statement(
        m_db,
        "SELECT id, title, preview, application, COALESCE(pin, ''), pinned, "
        "first_copied_at, copied_at, copy_count, has_text, has_image, has_files "
        "FROM history_items ORDER BY id DESC;"
    );

    std::vector<ClipboardItem> all;
    while (true) {
        if (cancelled()) {
            return {};
        }
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

    bool fuzzyResults = false;
    std::optional<std::wregex> regex;
    if (!query.empty() && (search_mode == 2 || search_mode == 3)) {
        try {
            regex.emplace(std::wstring(query));
        } catch (const std::regex_error&) {
            regex.reset();
        }
    }
    std::unordered_set<sqlite3_int64> indexed_ids;
    if (!query.empty() && query.size() >= 3 && (search_mode == 0 || search_mode == 3)) {
        for (const sqlite3_int64 id : SearchIndexIds(query, is_cancelled)) {
            if (cancelled()) {
                return {};
            }
            indexed_ids.insert(id);
        }
    }

    const auto matchesRegex = [&regex](std::wstring_view text) {
        if (!regex) {
            return false;
        }
        return std::regex_search(text.begin(), text.end(), *regex);
    };

    const auto itemIndexForId = [&all](sqlite3_int64 id) -> std::optional<size_t> {
        const auto found = std::lower_bound(
            all.begin(),
            all.end(),
            id,
            [](const ClipboardItem &item, sqlite3_int64 value) {
                return item.id > value;
            }
        );
        if (found == all.end() || found->id != id) {
            return std::nullopt;
        }
        return static_cast<size_t>(found - all.begin());
    };

    const auto considerFuzzyScore = [&query](std::optional<double> &best, std::wstring_view text) {
        if (const auto score = FuzzyScore(text, query)) {
            if (!best || *score < *best) {
                best = *score;
            }
        }
    };

    std::vector<size_t> selected;
    selected.reserve(all.size());
    const auto selectExact = [&]() {
        std::vector<unsigned char> document_matches(all.size(), 0);
        if (query.size() < 3) {
            ForEachSearchDocument([&](const SearchDocument &document) {
                const auto index = itemIndexForId(document.id);
                if (index.has_value()) {
                    document_matches[*index] = ContainsExact(document.body, query) ||
                        ContainsExact(document.paths, query);
                }
            }, is_cancelled);
            if (cancelled()) {
                return;
            }
        }
        for (size_t index = 0; index < all.size(); ++index) {
            if (cancelled()) {
                return;
            }
            const ClipboardItem &item = all[index];
            const bool matches_metadata = ContainsExact(item.title, query) ||
                ContainsExact(item.preview, query);
            const bool matches_payload = query.size() >= 3
                ? indexed_ids.contains(item.id)
                : document_matches[index] != 0;
            if (matches_metadata || matches_payload) {
                selected.push_back(index);
            }
        }
    };
    const auto selectRegex = [&]() {
        std::vector<unsigned char> document_matches(all.size(), 0);
        if (regex) {
            ForEachSearchDocument([&](const SearchDocument &document) {
                const auto index = itemIndexForId(document.id);
                if (index.has_value()) {
                    document_matches[*index] = matchesRegex(document.body) ||
                        matchesRegex(document.paths);
                }
            }, is_cancelled);
            if (cancelled()) {
                return;
            }
        }
        for (size_t index = 0; index < all.size(); ++index) {
            if (cancelled()) {
                return;
            }
            const ClipboardItem &item = all[index];
            if (matchesRegex(item.title) || matchesRegex(item.preview) ||
                document_matches[index] != 0) {
                selected.push_back(index);
            }
        }
    };
    const auto selectFuzzy = [&]() {
        fuzzyResults = true;
        std::vector<std::optional<double>> best_scores(all.size());
        for (size_t index = 0; index < all.size(); ++index) {
            if (cancelled()) {
                return;
            }
            considerFuzzyScore(best_scores[index], all[index].title);
            considerFuzzyScore(best_scores[index], all[index].preview);
        }
        ForEachSearchDocument([&](const SearchDocument &document) {
            const auto index = itemIndexForId(document.id);
            if (!index.has_value()) {
                return;
            }
            considerFuzzyScore(best_scores[*index], document.body);
            considerFuzzyScore(best_scores[*index], document.paths);
        }, is_cancelled);
        if (cancelled()) {
            return;
        }

        std::vector<std::pair<double, size_t>> fuzzy;
        fuzzy.reserve(all.size());
        for (size_t index = 0; index < all.size(); ++index) {
            if (cancelled()) {
                return;
            }
            if (best_scores[index].has_value()) {
                fuzzy.emplace_back(*best_scores[index], index);
            }
        }
        std::stable_sort(fuzzy.begin(), fuzzy.end(), [](const auto &lhs, const auto &rhs) {
            return lhs.first < rhs.first;
        });
        for (const auto &[score, index] : fuzzy) {
            selected.push_back(index);
        }
    };

    if (query.empty()) {
        for (size_t index = 0; index < all.size(); ++index) {
            if (cancelled()) {
                return {};
            }
            selected.push_back(index);
        }
    } else if (search_mode == 0) {
        selectExact();
    } else if (search_mode == 1) {
        selectFuzzy();
    } else if (search_mode == 2) {
        selectRegex();
    } else {
        selectExact();
        if (selected.empty()) {
            selectRegex();
        }
        if (selected.empty()) {
            selectFuzzy();
        }
    }

    std::vector<ClipboardItem> filtered;
    filtered.reserve(selected.size());
    for (const size_t index : selected) {
        if (cancelled()) {
            return {};
        }
        filtered.push_back(std::move(all[index]));
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

std::optional<ClipboardItem> Database::GetItem(sqlite3_int64 id, PayloadMode payload_mode) const {
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
    if (payload_mode != PayloadMode::Metadata) {
        LoadPayload(item, payload_mode);
    }
    return item;
}

std::vector<ClipboardItem> Database::GetPinnedItems(PayloadMode payload_mode) const {
    std::vector<ClipboardItem> pinned;
    for (ClipboardItem &metadata : SearchHistory({}, 0, 0, false)) {
        if (!metadata.pinned) {
            continue;
        }
        if (payload_mode == PayloadMode::Metadata) {
            pinned.push_back(std::move(metadata));
        } else if (auto item = GetItem(metadata.id, payload_mode)) {
            pinned.push_back(std::move(*item));
        }
    }
    return pinned;
}

void Database::LoadPayload(ClipboardItem &item, PayloadMode payload_mode) const {
    if (payload_mode == PayloadMode::Metadata) {
        return;
    }

    const char *sql = nullptr;
    if (payload_mode == PayloadMode::Full) {
        sql =
            "SELECT format, name, data FROM clipboard_formats "
            "WHERE item_id = ?1 ORDER BY sequence ASC;";
    } else if (item.has_image) {
        sql =
            "SELECT format, name, data FROM clipboard_formats "
            "WHERE item_id = ?1 AND ("
            "format IN (8, 17) OR name COLLATE NOCASE IN ("
            "'PNG', 'image/png', 'JFIF', 'image/jpeg', 'TIFF', 'image/tiff', "
            "'HEIC', 'image/heic')) "
            "ORDER BY CASE "
            "WHEN name COLLATE NOCASE IN ('PNG', 'image/png', 'JFIF', 'image/jpeg', "
            "'TIFF', 'image/tiff', 'HEIC', 'image/heic') THEN 0 "
            "WHEN format = 17 THEN 1 "
            "WHEN format = 8 THEN 2 ELSE 3 END, sequence ASC LIMIT 1;";
    } else if (item.has_text) {
        sql =
            "SELECT format, name, data FROM clipboard_formats "
            "WHERE item_id = ?1 AND (format IN (1, 13) OR name COLLATE NOCASE IN ("
            "'CF_TEXT', 'CF_UNICODETEXT')) "
            "ORDER BY CASE WHEN format = 13 OR name COLLATE NOCASE = 'CF_UNICODETEXT' "
            "THEN 0 ELSE 1 END, sequence ASC LIMIT 1;";
    } else {
        return;
    }

    Statement statement(m_db, sql);
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, item.id), "Unable to bind item id");
    while (true) {
        const int result = sqlite3_step(statement.get());
        if (result == SQLITE_DONE) {
            break;
        }
        if (result != SQLITE_ROW) {
            throw MakeSqliteError(m_db, "Unable to load clipboard formats");
        }

        ClipboardFormatData format;
        format.format = static_cast<UINT>(sqlite3_column_int64(statement.get(), 0));
        format.name = ColumnText16(statement.get(), 1);
        format.bytes = ColumnBlob(statement.get(), 2);
        item.data.push_back(std::move(format));
    }
}

void Database::DeleteItem(sqlite3_int64 id) const {
    auto transaction = BeginTransaction();
    Statement remove_index(m_db, "DELETE FROM history_fts WHERE rowid = ?1;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(remove_index.get(), 1, id), "Unable to bind search item id");
    if (sqlite3_step(remove_index.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to delete clipboard search index");
    }

    Statement statement(m_db, "DELETE FROM history_items WHERE id = ?1;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, id), "Unable to bind item id");
    if (sqlite3_step(statement.get()) != SQLITE_DONE) {
        throw MakeSqliteError(m_db, "Unable to delete history item");
    }
    transaction.Commit();
}

void Database::DeleteUnpinned() const {
    auto transaction = BeginTransaction();
    Exec("DELETE FROM history_fts WHERE rowid IN (SELECT id FROM history_items WHERE pinned = 0);");
    Exec("DELETE FROM history_items WHERE pinned = 0;");
    transaction.Commit();
}

void Database::DeleteAll() const {
    auto transaction = BeginTransaction();
    Exec("DELETE FROM history_fts;");
    Exec("DELETE FROM history_items;");
    transaction.Commit();
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
        ReplaceFormats(id, {data});
        ReplaceSearchIndex(id, text, {});
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
    return total;
}

void Database::MarkCopied(sqlite3_int64 id) const {
    Statement statement(m_db, "UPDATE history_items SET copied_at = ?1, copy_count = copy_count + 1 WHERE id = ?2;");
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 1, CurrentUnixMilliseconds()), "Unable to bind copy time");
    CheckSqliteResult(m_db, sqlite3_bind_int64(statement.get(), 2, id), "Unable to bind item id");
    if (sqlite3_step(statement.get()) != SQLITE_DONE) throw MakeSqliteError(m_db, "Unable to update copy count");
}
