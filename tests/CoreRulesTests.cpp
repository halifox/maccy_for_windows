#include "ClipboardRules.h"
#include "PinKeys.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace {

bool CheckApplicationMatching() {
    return ClipboardRules::MatchesApplication(
               L"C:\\Program Files\\Editor\\Editor.exe",
               L"c:\\program files\\editor\\editor.exe") &&
        ClipboardRules::MatchesApplication(
               L"C:\\Program Files\\Editor\\Editor.exe",
               L"EDITOR.EXE") &&
        !ClipboardRules::MatchesApplication(
               L"C:\\Program Files\\Editor\\Editor.exe",
               L"Other\\Editor.exe") &&
        !ClipboardRules::MatchesApplication(L"", L"editor.exe");
}

bool CheckBoundedDecoding() {
    constexpr size_t limit = 4;
    const std::vector<unsigned char> unicode_bytes = {
        'A', 0, 'B', 0, 'C', 0, 'D', 0, 'E', 0, 0, 0,
    };
    const std::vector<unsigned char> nul_unicode_bytes = {
        'A', 0, 0, 0, 'B', 0, 0, 0,
    };
    const std::vector<unsigned char> ansi_bytes = {'A', 'B', 'C', 'D', 'E', 0};

    return ClipboardRules::DecodeUnicodeText(unicode_bytes, limit) == L"ABCD" &&
        ClipboardRules::DecodeUnicodeText(unicode_bytes, 0).empty() &&
        ClipboardRules::DecodeUnicodeText(nul_unicode_bytes, limit) == L"A" &&
        ClipboardRules::DecodeAnsiText(ansi_bytes, limit) == L"ABCD" &&
        ClipboardRules::DecodeAnsiText(ansi_bytes, 0).empty() &&
        ClipboardRules::DecodeAnsiText(ansi_bytes) == L"ABCDE";
}

bool CheckSearchParts() {
    const std::wstring text = L"clipboard text";
    std::vector<unsigned char> bytes((text.size() + 1) * sizeof(wchar_t));
    std::memcpy(bytes.data(), text.c_str(), bytes.size());

    const ClipboardFormatData format{L"CF_UNICODETEXT", CF_UNICODETEXT, std::move(bytes)};
    const ClipboardRules::SearchParts parts = ClipboardRules::SearchPartsFromFormats({format});
    return parts.body == text && parts.paths.empty();
}

bool CheckPinKeys() {
    AppSettings settings;
    std::vector<ClipboardItem> pins;
    ClipboardItem pin;
    pin.id = 1;
    pin.pinned = true;
    pin.pin = L"b";
    pins.push_back(pin);

    const auto available = PinKeyPolicy::Available(pins, settings);
    return !PinKeyPolicy::IsValid(L"b", pins, settings, 0) &&
        PinKeyPolicy::IsValid(L"b", pins, settings, 1) &&
        PinKeyPolicy::Next(pins, settings) == L"c" &&
        std::find(available.begin(), available.end(), L'p') == available.end();
}

} // namespace

int main() {
    if (!CheckApplicationMatching()) return 1;
    if (!CheckSearchParts()) return 2;
    if (!CheckPinKeys()) return 3;
    if (!CheckBoundedDecoding()) return 4;
    return 0;
}
