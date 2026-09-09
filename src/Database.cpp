#include "Database.h"

#include <stdexcept>
#include <string>

namespace {

std::runtime_error MakeSqliteError(sqlite3 *db, const char *operation) {
    std::string message = operation;
    if (db != nullptr) {
        message += ": ";
        message += sqlite3_errmsg(db);
    }
    return std::runtime_error(message);
}

} // namespace

Database::Database(const std::filesystem::path &path) {
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
        Exec(
            "CREATE TABLE IF NOT EXISTS app_state ("
            "key TEXT PRIMARY KEY,"
            "value TEXT NOT NULL"
            ");"
        );
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
