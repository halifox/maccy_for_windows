#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <cstdint>
#include <string>

class Database;

enum class PopupPosition {
    Cursor = 0,
    StatusItem = 1,
    WindowCenter = 2,
    ScreenCenter = 3,
    LastPosition = 4,
};

enum class PinPosition {
    Top = 0,
    Bottom = 1,
};

enum class HighlightMatch {
    Color = 0,
    Bold = 1,
    Italic = 2,
    Underline = 3,
};

enum class SearchMode {
    Exact = 0,
    Fuzzy = 1,
    Regexp = 2,
    Mixed = 3,
};

enum class SearchVisibility {
    Always = 0,
    DuringSearch = 1,
};

struct HotKeyConfig {
    UINT modifiers = MOD_CONTROL | MOD_SHIFT;
    UINT virtual_key = VK_SPACE;
};

struct AppSettings {
    bool launch_at_login = false;
    bool check_for_updates = false;

    bool clear_on_quit = false;
    bool clear_system_clipboard = false;
    bool respect_windows_clipboard_history_markers = true;
    bool ignore_all_apps_except_listed = false;
    bool ignore_events = false;
    bool ignore_only_next_event = false;

    bool save_files = true;
    bool save_images = true;
    bool save_text = true;
    int history_size = AppConstants::DB::kDefaultHistorySize;
    int sort_by = 0; // last copied, first copied, number of copies

    PopupPosition popup_position = PopupPosition::Cursor;
    int popup_screen = 0; // 0 = active screen, otherwise monitor index + 1
    PinPosition pin_to = PinPosition::Top;
    int window_width = AppConstants::UI::kDefaultWindowWidth;
    int window_height = AppConstants::UI::kDefaultWindowHeight;
    int image_max_height = AppConstants::UI::kDefaultImageMaxHeight;
    bool open_preview_automatically = true;
    int preview_delay = 1500;
    HighlightMatch highlight_match = HighlightMatch::Bold;
    std::wstring menu_icon = L"maccy";
    bool show_in_status_bar = true;
    bool show_search = true;
    SearchVisibility search_visibility = SearchVisibility::Always;
    bool show_title = true;
    bool show_footer = true;
    bool show_special_symbols = true;
    bool show_application_icons = false;
    bool show_hex_color_swatch = true;

    SearchMode search_mode = SearchMode::Exact;
    bool paste_by_default = false;
    bool remove_formatting_by_default = false;

    HotKeyConfig open_hotkey{};
    HotKeyConfig pin_hotkey{MOD_CONTROL, 'P'};
    HotKeyConfig delete_hotkey{MOD_CONTROL, 'D'};
    HotKeyConfig preview_hotkey{MOD_CONTROL, VK_SPACE};

    int popup_x = 0;
    int popup_y = 0;

    static AppSettings Load(const Database &database);
    void Save(const Database &database) const;
};

std::wstring HotKeyToText(const HotKeyConfig &hotkey);
bool IsHotKeyPressed(const HotKeyConfig &hotkey, WPARAM virtual_key);
bool SameHotKey(const HotKeyConfig &lhs, const HotKeyConfig &rhs);
bool SetLaunchAtLogin(bool enabled);
bool IsLaunchAtLogin();
std::wstring FormatByteCount(std::uintmax_t bytes);
