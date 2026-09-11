#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <cstdint>
#include <string>

class Database;

enum class PopupPosition {
    Cursor = 0,
    StatusItem = 1,
    WindowCenter = 2,
    ScreenCenter = 3,
    LastPosition = 4,
    WindowTopLeft = 5,
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
    bool check_for_updates = true;

    bool clear_on_quit = false;
    bool clear_system_clipboard = false;
    bool ignore_all_apps_except_listed = false;
    bool ignore_events = false;
    bool ignore_only_next_event = false;

    bool save_files = true;
    bool save_images = true;
    bool save_text = true;
    int history_size = 200;
    int sort_by = 0; // last copied, first copied, number of copies

    PopupPosition popup_position = PopupPosition::WindowTopLeft;
    int popup_screen = 0; // 0 = active screen, otherwise monitor index + 1
    PinPosition pin_to = PinPosition::Top;
    int window_width = 450;
    int window_height = 800;
    int image_max_height = 40;
    bool open_preview_automatically = true;
    int preview_delay = 1500;
    int preview_width = 450;
    HighlightMatch highlight_match = HighlightMatch::Bold;
    std::wstring menu_icon = L"maccy";
    bool show_in_status_bar = true;
    bool show_recent_copy_in_menu_bar = false;
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
    HotKeyConfig pin_hotkey{MOD_ALT, 'P'};
    HotKeyConfig delete_hotkey{MOD_ALT, VK_BACK};
    HotKeyConfig preview_hotkey{MOD_CONTROL, VK_SPACE};

    int popup_x = 0;
    int popup_y = 0;

    static AppSettings Load(const Database &database);
    void Save(const Database &database) const;
};

std::wstring HotKeyToText(const HotKeyConfig &hotkey);
HotKeyConfig HotKeyFromControl(HWND control);
void SetHotKeyControl(HWND control, const HotKeyConfig &hotkey);
bool IsHotKeyPressed(const HotKeyConfig &hotkey, WPARAM virtual_key);
bool SetLaunchAtLogin(bool enabled);
bool IsLaunchAtLogin();
std::wstring FormatByteCount(std::uintmax_t bytes);
