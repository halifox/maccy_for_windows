#pragma once

#include <filesystem>
#include <string_view>

#include <sqlite3.h>

class Database {
public:
    explicit Database(const std::filesystem::path &path);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    void Exec(std::string_view sql) const;

    sqlite3 *Handle() const noexcept { return m_db; }

private:
    sqlite3 *m_db = nullptr;
};
