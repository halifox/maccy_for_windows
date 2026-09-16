#pragma once

#include "Database.h"

#include <cstdint>
#include <filesystem>
#include <vector>

class ClipboardPayloadStore {
public:
    explicit ClipboardPayloadStore(std::filesystem::path root);

    ClipboardPayloadStore(const ClipboardPayloadStore&) = delete;
    ClipboardPayloadStore& operator=(const ClipboardPayloadStore&) = delete;

    void Save(sqlite3_int64 item_id, const std::vector<ClipboardFormatData>& data) const;
    std::vector<ClipboardFormatData> Load(sqlite3_int64 item_id) const;
    void Remove(sqlite3_int64 item_id) const;
    void Clear() const;
    std::uintmax_t StorageBytes() const;

private:
    std::filesystem::path PathFor(sqlite3_int64 item_id) const;

    std::filesystem::path m_root;
};
