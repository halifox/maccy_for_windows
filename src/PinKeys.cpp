#include "PinKeys.h"

#include <algorithm>
#include <cwctype>

namespace {

constexpr std::wstring_view kSupportedKeys = L"bcdefghijklmnoprstuxy";

wchar_t NormalizeKey(wchar_t key) {
    return static_cast<wchar_t>(std::towlower(key));
}

bool MatchesVirtualKey(wchar_t key, UINT virtual_key) {
    return static_cast<UINT>(std::towupper(key)) == virtual_key;
}

bool IsReservedByAction(wchar_t key, const AppSettings &settings) {
    return MatchesVirtualKey(key, settings.pin_hotkey.virtual_key) ||
        MatchesVirtualKey(key, settings.delete_hotkey.virtual_key) ||
        MatchesVirtualKey(key, settings.preview_hotkey.virtual_key);
}

bool IsAssignedToAnotherPin(
    wchar_t key,
    const std::vector<ClipboardItem> &pins,
    sqlite3_int64 selected_id
) {
    return std::any_of(pins.begin(), pins.end(), [&](const ClipboardItem &item) {
        return item.id != selected_id && item.pinned && item.pin.size() == 1 &&
            NormalizeKey(item.pin.front()) == key;
    });
}

}

namespace PinKeyPolicy {

std::vector<wchar_t> Available(
    const std::vector<ClipboardItem> &pins,
    const AppSettings &settings,
    sqlite3_int64 selected_id
) {
    std::vector<wchar_t> keys;
    for (const wchar_t key : kSupportedKeys) {
        if (IsReservedByAction(key, settings) || IsAssignedToAnotherPin(key, pins, selected_id)) {
            continue;
        }
        keys.push_back(key);
    }
    return keys;
}

bool IsValid(
    std::wstring_view key,
    const std::vector<ClipboardItem> &pins,
    const AppSettings &settings,
    sqlite3_int64 selected_id
) {
    if (key.size() != 1) {
        return false;
    }

    const wchar_t normalized = NormalizeKey(key.front());
    const auto selected = std::find_if(pins.begin(), pins.end(), [&](const ClipboardItem &item) {
        return item.id == selected_id;
    });
    if (selected != pins.end() && selected->pin.size() == 1 &&
        NormalizeKey(selected->pin.front()) == normalized) {
        return true;
    }

    const auto available = Available(pins, settings, selected_id);
    return std::find(available.begin(), available.end(), normalized) != available.end();
}

std::wstring Next(
    const std::vector<ClipboardItem> &pins,
    const AppSettings &settings
) {
    const auto available = Available(pins, settings);
    return available.empty() ? std::wstring{} : std::wstring(1, available.front());
}

}
