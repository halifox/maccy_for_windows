#pragma once

#include "PlatformConfig.h"

#include <sqlite3.h>

#include <string>
#include <vector>

// Clipboard data is the application domain model.  It deliberately does not
// depend on either a window or a storage implementation.
struct ClipboardFormatData {
    std::wstring name;
    UINT format = 0;
    std::vector<unsigned char> bytes;
};

struct ClipboardSnapshot {
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

enum class PayloadMode {
    Metadata,
    Preview,
    Full,
};

enum class IgnoreListKind {
    Applications,
    Formats,
    Regexps,
};
