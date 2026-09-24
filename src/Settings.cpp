#include "Settings.h"

#include "Database.h"

#include <commctrl.h>

#include <algorithm>
#include <array>
#include <cwchar>
#include <stdexcept>
#include <sstream>

namespace {

std::wstring Read(const Database &database, std::wstring_view key, std::wstring fallback) {
    if (const auto value = database.GetSetting(key)) {
        return *value;
    }
    return fallback;
}

bool ReadBool(const Database &database, std::wstring_view key, bool fallback) {
    const std::wstring value = Read(database, key, fallback ? L"1" : L"0");
    return value == L"1" || value == L"true" || value == L"TRUE";
}

int ReadInt(const Database &database, std::wstring_view key, int fallback, int minimum, int maximum) {
    const std::wstring value = Read(database, key, std::to_wstring(fallback));
    try {
        return std::clamp(std::stoi(value), minimum, maximum);
    } catch (...) {
        return fallback;
    }
}

HotKeyConfig ReadHotKey(
    const Database &database,
    std::wstring_view modifier_key,
    std::wstring_view virtual_key_key,
    HotKeyConfig fallback
) {
    HotKeyConfig result;
    result.modifiers = static_cast<UINT>(ReadInt(database, modifier_key, static_cast<int>(fallback.modifiers), 0, 0xFFFF));
    result.virtual_key = static_cast<UINT>(ReadInt(database, virtual_key_key, static_cast<int>(fallback.virtual_key), 1, 0xFF));
    return result;
}

void WriteBool(const Database &database, std::wstring_view key, bool value) {
    database.SetSetting(key, value ? L"1" : L"0");
}

void WriteInt(const Database &database, std::wstring_view key, int value) {
    database.SetSetting(key, std::to_wstring(value));
}

void WriteHotKey(
    const Database &database,
    std::wstring_view modifier_key,
    std::wstring_view virtual_key_key,
    const HotKeyConfig &hotkey
) {
    WriteInt(database, modifier_key, static_cast<int>(hotkey.modifiers));
    WriteInt(database, virtual_key_key, static_cast<int>(hotkey.virtual_key));
}

std::wstring KeyName(UINT virtual_key) {
    if (virtual_key >= L'A' && virtual_key <= L'Z') {
        return std::wstring(1, static_cast<wchar_t>(virtual_key));
    }
    if (virtual_key >= VK_F1 && virtual_key <= VK_F24) {
        return L"F" + std::to_wstring(virtual_key - VK_F1 + 1);
    }
    switch (virtual_key) {
    case VK_SPACE:
        return L"Space";
    case VK_BACK:
        return L"Backspace";
    case VK_DELETE:
        return L"Delete";
    case VK_RETURN:
        return L"Enter";
    case VK_TAB:
        return L"Tab";
    case VK_ESCAPE:
        return L"Esc";
    case VK_LEFT:
        return L"Left";
    case VK_RIGHT:
        return L"Right";
    case VK_UP:
        return L"Up";
    case VK_DOWN:
        return L"Down";
    default:
        return L"VK-" + std::to_wstring(virtual_key);
    }
}

} // namespace

AppSettings AppSettings::Load(const Database &database) {
    AppSettings settings;
    const auto launch = database.GetSetting(L"general.launchAtLogin");
    settings.launch_at_login = launch ? (*launch == L"1") : IsLaunchAtLogin();
    settings.check_for_updates = ReadBool(database, L"general.checkForUpdates", settings.check_for_updates);
    settings.clear_on_quit = ReadBool(database, L"advanced.clearOnQuit", settings.clear_on_quit);
    settings.clear_system_clipboard = ReadBool(database, L"advanced.clearSystemClipboard", settings.clear_system_clipboard);
    settings.respect_windows_clipboard_history_markers = ReadBool(
        database,
        L"advanced.respectWindowsClipboardHistoryMarkers",
        settings.respect_windows_clipboard_history_markers
    );
    settings.ignore_all_apps_except_listed = ReadBool(
        database,
        L"ignore.allAppsExceptListed",
        settings.ignore_all_apps_except_listed
    );
    settings.ignore_events = ReadBool(database, L"advanced.ignoreEvents", settings.ignore_events);
    settings.ignore_only_next_event = ReadBool(
        database,
        L"advanced.ignoreOnlyNextEvent",
        settings.ignore_only_next_event
    );

    settings.save_files = ReadBool(database, L"storage.saveFiles", settings.save_files);
    settings.save_images = ReadBool(database, L"storage.saveImages", settings.save_images);
    settings.save_text = ReadBool(database, L"storage.saveText", settings.save_text);
    settings.history_size = ReadInt(database, L"storage.historySize", settings.history_size, 1, 999);
    settings.sort_by = ReadInt(database, L"storage.sortBy", settings.sort_by, 0, 2);

    settings.popup_position = static_cast<PopupPosition>(ReadInt(
        database,
        L"appearance.popupPosition",
        static_cast<int>(settings.popup_position),
        0,
        4
    ));
    settings.popup_screen = ReadInt(database, L"appearance.popupScreen", settings.popup_screen, 0, 64);
    settings.pin_to = static_cast<PinPosition>(ReadInt(
        database,
        L"appearance.pinTo",
        static_cast<int>(settings.pin_to),
        0,
        1
    ));
    settings.window_width = ReadInt(
        database,
        L"appearance.windowWidth",
        settings.window_width,
        AppConstants::UI::kMinimumPopupWidth,
        AppConstants::UI::kMaximumPopupWidth
    );
    settings.window_height = ReadInt(
        database,
        L"appearance.windowHeight",
        settings.window_height,
        AppConstants::UI::kMinimumPopupHeight,
        AppConstants::UI::kMaximumPopupHeight
    );
    settings.image_max_height = ReadInt(database, L"appearance.imageMaxHeight", settings.image_max_height, 1, 200);
    settings.open_preview_automatically = ReadBool(
        database,
        L"appearance.openPreviewAutomatically",
        settings.open_preview_automatically
    );
    settings.preview_delay = ReadInt(database, L"appearance.previewDelay", settings.preview_delay, 200, 100000);
    settings.highlight_match = static_cast<HighlightMatch>(ReadInt(
        database,
        L"appearance.highlightMatch",
        static_cast<int>(settings.highlight_match),
        0,
        3
    ));
    settings.menu_icon = Read(database, L"appearance.menuIcon", settings.menu_icon);
    settings.show_in_status_bar = ReadBool(database, L"appearance.showInStatusBar", settings.show_in_status_bar);
    settings.show_search = ReadBool(database, L"appearance.showSearch", settings.show_search);
    settings.search_visibility = static_cast<SearchVisibility>(ReadInt(
        database,
        L"appearance.searchVisibility",
        static_cast<int>(settings.search_visibility),
        0,
        1
    ));
    settings.show_title = ReadBool(database, L"appearance.showTitle", settings.show_title);
    settings.show_footer = ReadBool(database, L"appearance.showFooter", settings.show_footer);
    settings.show_special_symbols = ReadBool(
        database,
        L"appearance.showSpecialSymbols",
        settings.show_special_symbols
    );
    settings.show_application_icons = ReadBool(
        database,
        L"appearance.showApplicationIcons",
        settings.show_application_icons
    );
    settings.show_hex_color_swatch = ReadBool(
        database,
        L"appearance.showHexColorSwatch",
        settings.show_hex_color_swatch
    );

    settings.search_mode = static_cast<SearchMode>(ReadInt(
        database,
        L"general.searchMode",
        static_cast<int>(settings.search_mode),
        0,
        3
    ));
    settings.paste_by_default = ReadBool(database, L"general.pasteByDefault", settings.paste_by_default);
    settings.remove_formatting_by_default = ReadBool(
        database,
        L"general.removeFormattingByDefault",
        settings.remove_formatting_by_default
    );

    settings.open_hotkey = ReadHotKey(
        database,
        L"hotkey.open.modifiers",
        L"hotkey.open.virtualKey",
        settings.open_hotkey
    );
    settings.pin_hotkey = ReadHotKey(
        database,
        L"hotkey.pin.modifiers",
        L"hotkey.pin.virtualKey",
        settings.pin_hotkey
    );
    settings.delete_hotkey = ReadHotKey(
        database,
        L"hotkey.delete.modifiers",
        L"hotkey.delete.virtualKey",
        settings.delete_hotkey
    );
    settings.preview_hotkey = ReadHotKey(
        database,
        L"hotkey.preview.modifiers",
        L"hotkey.preview.virtualKey",
        settings.preview_hotkey
    );

    settings.popup_x = ReadInt(database, L"appearance.windowX", settings.popup_x, -32768, 32768);
    settings.popup_y = ReadInt(database, L"appearance.windowY", settings.popup_y, -32768, 32768);
    return settings;
}

void AppSettings::Save(const Database &database) const {
    auto transaction = database.BeginTransaction();
        WriteBool(database, L"general.launchAtLogin", launch_at_login);
        WriteBool(database, L"general.checkForUpdates", check_for_updates);
        WriteBool(database, L"advanced.clearOnQuit", clear_on_quit);
        WriteBool(database, L"advanced.clearSystemClipboard", clear_system_clipboard);
        WriteBool(
            database,
            L"advanced.respectWindowsClipboardHistoryMarkers",
            respect_windows_clipboard_history_markers
        );
        WriteBool(database, L"ignore.allAppsExceptListed", ignore_all_apps_except_listed);
        WriteBool(database, L"advanced.ignoreEvents", ignore_events);
        WriteBool(database, L"advanced.ignoreOnlyNextEvent", ignore_only_next_event);

        WriteBool(database, L"storage.saveFiles", save_files);
        WriteBool(database, L"storage.saveImages", save_images);
        WriteBool(database, L"storage.saveText", save_text);
        WriteInt(database, L"storage.historySize", history_size);
        WriteInt(database, L"storage.sortBy", sort_by);

        WriteInt(database, L"appearance.popupPosition", static_cast<int>(popup_position));
        WriteInt(database, L"appearance.popupScreen", popup_screen);
        WriteInt(database, L"appearance.pinTo", static_cast<int>(pin_to));
        WriteInt(database, L"appearance.windowWidth", window_width);
        WriteInt(database, L"appearance.windowHeight", window_height);
        WriteInt(database, L"appearance.imageMaxHeight", image_max_height);
        WriteBool(database, L"appearance.openPreviewAutomatically", open_preview_automatically);
        WriteInt(database, L"appearance.previewDelay", preview_delay);
        WriteInt(database, L"appearance.highlightMatch", static_cast<int>(highlight_match));
        database.SetSetting(L"appearance.menuIcon", menu_icon);
        WriteBool(database, L"appearance.showInStatusBar", show_in_status_bar);
        WriteBool(database, L"appearance.showSearch", show_search);
        WriteInt(database, L"appearance.searchVisibility", static_cast<int>(search_visibility));
        WriteBool(database, L"appearance.showTitle", show_title);
        WriteBool(database, L"appearance.showFooter", show_footer);
        WriteBool(database, L"appearance.showSpecialSymbols", show_special_symbols);
        WriteBool(database, L"appearance.showApplicationIcons", show_application_icons);
        WriteBool(database, L"appearance.showHexColorSwatch", show_hex_color_swatch);

        WriteInt(database, L"general.searchMode", static_cast<int>(search_mode));
        WriteBool(database, L"general.pasteByDefault", paste_by_default);
        WriteBool(database, L"general.removeFormattingByDefault", remove_formatting_by_default);

        WriteHotKey(database, L"hotkey.open.modifiers", L"hotkey.open.virtualKey", open_hotkey);
        WriteHotKey(database, L"hotkey.pin.modifiers", L"hotkey.pin.virtualKey", pin_hotkey);
        WriteHotKey(database, L"hotkey.delete.modifiers", L"hotkey.delete.virtualKey", delete_hotkey);
        WriteHotKey(database, L"hotkey.preview.modifiers", L"hotkey.preview.virtualKey", preview_hotkey);

        WriteInt(database, L"appearance.windowX", popup_x);
        WriteInt(database, L"appearance.windowY", popup_y);
    transaction.Commit();

    if (!SetLaunchAtLogin(launch_at_login)) {
        throw std::runtime_error("Unable to update launch-at-login setting");
    }
}

std::wstring HotKeyToText(const HotKeyConfig &hotkey) {
    std::wstring text;
    if ((hotkey.modifiers & MOD_CONTROL) != 0) {
        text += L"Ctrl+";
    }
    if ((hotkey.modifiers & MOD_ALT) != 0) {
        text += L"Alt+";
    }
    if ((hotkey.modifiers & MOD_SHIFT) != 0) {
        text += L"Shift+";
    }
    if ((hotkey.modifiers & MOD_WIN) != 0) {
        text += L"Win+";
    }
    return text + KeyName(hotkey.virtual_key);
}

bool IsHotKeyPressed(const HotKeyConfig &hotkey, WPARAM virtual_key) {
    if (virtual_key != hotkey.virtual_key) {
        return false;
    }
    const auto pressed = [](int key) {
        return (::GetKeyState(key) & 0x8000) != 0;
    };
    const bool control = pressed(VK_CONTROL);
    const bool alt = pressed(VK_MENU);
    const bool shift = pressed(VK_SHIFT);
    const bool win = pressed(VK_LWIN) || pressed(VK_RWIN);
    return control == ((hotkey.modifiers & MOD_CONTROL) != 0) &&
        alt == ((hotkey.modifiers & MOD_ALT) != 0) &&
        shift == ((hotkey.modifiers & MOD_SHIFT) != 0) &&
        win == ((hotkey.modifiers & MOD_WIN) != 0);
}

bool SameHotKey(const HotKeyConfig &lhs, const HotKeyConfig &rhs) {
    return lhs.modifiers == rhs.modifiers && lhs.virtual_key == rhs.virtual_key;
}

bool SetLaunchAtLogin(bool enabled) {
    constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY key = nullptr;
    if (enabled) {
        if (RegCreateKeyExW(
            HKEY_CURRENT_USER,
            kRunKey,
            0,
            nullptr,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            nullptr,
            &key,
            nullptr
        ) != ERROR_SUCCESS) {
            return false;
        }
    } else {
        const LONG open_result = RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_SET_VALUE, &key);
        if (open_result == ERROR_FILE_NOT_FOUND || open_result == ERROR_PATH_NOT_FOUND) {
            return true;
        }
        if (open_result != ERROR_SUCCESS) {
            return false;
        }
    }

    LONG result = ERROR_SUCCESS;
    if (enabled) {
        std::array<wchar_t, MAX_PATH> path{};
        const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
        if (length == 0 || length >= path.size()) {
            result = ERROR_FILE_NOT_FOUND;
        } else {
            const std::wstring command = L"\"" + std::wstring(path.data(), length) + L"\"";
            result = RegSetValueExW(
                key,
                L"maccy",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE *>(command.c_str()),
                static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t))
            );
        }
    } else {
        result = RegDeleteValueW(key, L"maccy");
        if (result == ERROR_FILE_NOT_FOUND) {
            result = ERROR_SUCCESS;
        }
    }
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

bool IsLaunchAtLogin() {
    constexpr wchar_t kRunKey[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return false;
    }
    const LONG result = RegQueryValueExW(key, L"maccy", nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(key);
    return result == ERROR_SUCCESS;
}

std::wstring FormatByteCount(std::uintmax_t bytes) {
    constexpr std::uintmax_t kKiB = 1024;
    constexpr std::uintmax_t kMiB = kKiB * 1024;
    constexpr std::uintmax_t kGiB = kMiB * 1024;
    if (bytes >= kGiB) {
        return std::to_wstring(bytes / kGiB) + L" GB";
    }
    if (bytes >= kMiB) {
        return std::to_wstring(bytes / kMiB) + L" MB";
    }
    if (bytes >= kKiB) {
        return std::to_wstring(bytes / kKiB) + L" KB";
    }
    return std::to_wstring(bytes) + L" B";
}
