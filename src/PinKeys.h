#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "ClipboardData.h"
#include "Settings.h"

namespace PinKeyPolicy {

std::vector<wchar_t> Available(
    const std::vector<ClipboardItem> &pins,
    const AppSettings &settings,
    sqlite3_int64 selected_id = 0
);

bool IsValid(
    std::wstring_view key,
    const std::vector<ClipboardItem> &pins,
    const AppSettings &settings,
    sqlite3_int64 selected_id = 0
);

std::wstring Next(
    const std::vector<ClipboardItem> &pins,
    const AppSettings &settings
);

}
