#include "ClipboardPayloadStore.h"

#include <fstream>
#include <limits>
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
void WriteValue(std::ofstream& stream, T value) {
    stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
    if (!stream) {
        throw std::runtime_error("Unable to write clipboard payload");
    }
}

template <typename T>
T ReadValue(std::ifstream& stream) {
    T value{};
    stream.read(reinterpret_cast<char*>(&value), sizeof(value));
    if (!stream) {
        throw std::runtime_error("Unable to read clipboard payload");
    }
    return value;
}

void WriteBytes(std::ofstream& stream, const void* bytes, std::size_t size) {
    if (size == 0) {
        return;
    }
    stream.write(static_cast<const char*>(bytes), static_cast<std::streamsize>(size));
    if (!stream) {
        throw std::runtime_error("Unable to write clipboard payload");
    }
}

void ReadBytes(std::ifstream& stream, void* bytes, std::size_t size) {
    if (size == 0) {
        return;
    }
    stream.read(static_cast<char*>(bytes), static_cast<std::streamsize>(size));
    if (!stream) {
        throw std::runtime_error("Unable to read clipboard payload");
    }
}

std::uint32_t NameByteCount(const std::wstring& name) {
    if (name.size() > std::numeric_limits<std::uint32_t>::max() / sizeof(wchar_t)) {
        throw std::runtime_error("Clipboard format name is too large");
    }
    return static_cast<std::uint32_t>(name.size() * sizeof(wchar_t));
}

std::filesystem::path TemporaryPath(const std::filesystem::path& path) {
    return path.wstring() + L".tmp";
}

} // namespace

ClipboardPayloadStore::ClipboardPayloadStore(std::filesystem::path root)
    : m_root(std::move(root)) {
    std::error_code error;
    std::filesystem::create_directories(m_root, error);
    if (error) {
        throw std::system_error(error, "Unable to create clipboard payload directory");
    }
}

std::filesystem::path ClipboardPayloadStore::PathFor(sqlite3_int64 item_id) const {
    if (item_id <= 0) {
        throw std::runtime_error("Invalid clipboard item id");
    }
    return m_root / (std::to_wstring(item_id) + L".payload");
}

void ClipboardPayloadStore::Save(
    sqlite3_int64 item_id,
    const std::vector<ClipboardFormatData>& data
) const {
    if (data.size() > kMaximumPayloadRecords) {
        throw std::runtime_error("Too many clipboard formats");
    }

    std::uint64_t total_bytes = 0;
    for (const ClipboardFormatData& item : data) {
        const std::uint32_t name_bytes = NameByteCount(item.name);
        if (name_bytes > kMaximumFormatNameBytes ||
            item.bytes.size() > kMaximumPayloadBytes ||
            total_bytes > kMaximumPayloadBytes - item.bytes.size()) {
            throw std::runtime_error("Clipboard payload is too large");
        }
        total_bytes += item.bytes.size();
    }

    const std::filesystem::path path = PathFor(item_id);
    const std::filesystem::path temporary = TemporaryPath(path);
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    if (!stream) {
        throw std::runtime_error("Unable to open clipboard payload file");
    }

    try {
        WriteValue(stream, kPayloadMagic);
        WriteValue(stream, kPayloadVersion);
        WriteValue(stream, static_cast<std::uint32_t>(data.size()));
        for (const ClipboardFormatData& item : data) {
            const std::uint32_t name_bytes = NameByteCount(item.name);
            WriteValue(stream, static_cast<std::uint32_t>(item.format));
            WriteValue(stream, name_bytes);
            WriteValue(stream, static_cast<std::uint64_t>(item.bytes.size()));
            WriteBytes(stream, item.name.data(), name_bytes);
            WriteBytes(stream, item.bytes.data(), item.bytes.size());
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

std::vector<ClipboardFormatData> ClipboardPayloadStore::Load(sqlite3_int64 item_id) const {
    const std::filesystem::path path = PathFor(item_id);
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open clipboard payload file");
    }

    const std::uint32_t magic = ReadValue<std::uint32_t>(stream);
    const std::uint32_t version = ReadValue<std::uint32_t>(stream);
    const std::uint32_t count = ReadValue<std::uint32_t>(stream);
    if (magic != kPayloadMagic || version != kPayloadVersion || count > kMaximumPayloadRecords) {
        throw std::runtime_error("Invalid clipboard payload file");
    }

    std::vector<ClipboardFormatData> data;
    data.reserve(count);
    std::uint64_t total_bytes = 0;
    for (std::uint32_t index = 0; index < count; ++index) {
        ClipboardFormatData item;
        item.format = static_cast<UINT>(ReadValue<std::uint32_t>(stream));
        const std::uint32_t name_bytes = ReadValue<std::uint32_t>(stream);
        const std::uint64_t payload_bytes = ReadValue<std::uint64_t>(stream);
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
        ReadBytes(stream, item.name.data(), name_bytes);
        ReadBytes(stream, item.bytes.data(), item.bytes.size());
        total_bytes += payload_bytes;
        data.push_back(std::move(item));
    }
    return data;
}

void ClipboardPayloadStore::Remove(sqlite3_int64 item_id) const {
    const std::filesystem::path path = PathFor(item_id);
    std::error_code error;
    std::filesystem::remove(path, error);
    if (error) {
        throw std::system_error(error, "Unable to remove clipboard payload file");
    }
    std::filesystem::remove(TemporaryPath(path), error);
    if (error) {
        throw std::system_error(error, "Unable to remove temporary clipboard payload file");
    }
}

void ClipboardPayloadStore::Clear() const {
    std::error_code error;
    std::filesystem::remove_all(m_root, error);
    if (error) {
        throw std::system_error(error, "Unable to clear clipboard payload directory");
    }
    std::filesystem::create_directories(m_root, error);
    if (error) {
        throw std::system_error(error, "Unable to recreate clipboard payload directory");
    }
}

std::uintmax_t ClipboardPayloadStore::StorageBytes() const {
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
