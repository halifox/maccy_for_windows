#include "ClipboardRules.h"
#include "PinKeys.h"

#include <algorithm>
#include <cstring>
#include <regex>
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
        PinKeyPolicy::Next(pins, settings) == L"0" &&
        PinKeyPolicy::IsValid(L"9", pins, settings, 0) &&
        std::find(available.begin(), available.end(), L'p') == available.end();
}

bool CheckStripMarkup() {
    const std::wstring html =
        L"Version:1.0\r\n\r\n"
        L"<html><script>ignore()</script><body>A&nbsp;&amp;&#65;&#x42;<style>x{}</style></body></html>";
    const std::wstring rtf = L"{\\rtf1\\ansi{\\b hi}\\par\\u65 ?\\'42}";
    return ClipboardRules::StripHtml(html).find(L"A &AB") != std::wstring::npos &&
        ClipboardRules::StripHtml(html).find(L"ignore") == std::wstring::npos &&
        ClipboardRules::StripRtf(rtf).find(L"hi") != std::wstring::npos &&
        ClipboardRules::StripRtf(rtf).find(L"A") != std::wstring::npos;
}

bool CheckRegexBoundAndIgnoreAliases() {
    try {
        const std::wregex expression(L"secret");
        std::wstring haystack(ClipboardRules::Limits::kMaximumRegexHaystackCharacters + 8, L'x');
        haystack.replace(0, 6, L"secret");
        if (!ClipboardRules::RegexSearchBounded(expression, haystack)) {
            return false;
        }
        haystack.assign(ClipboardRules::Limits::kMaximumRegexHaystackCharacters + 8, L'x');
        haystack.replace(ClipboardRules::Limits::kMaximumRegexHaystackCharacters + 1, 6, L"secret");
        if (ClipboardRules::RegexSearchBounded(expression, haystack)) {
            return false;
        }
    } catch (const std::regex_error &) {
        return false;
    }
    return ClipboardRules::MatchesIgnoredFormatName(
               L"Clipboard Viewer Ignore",
               L"CF_CLIPBOARD_VIEWER_IGNORE"
           ) &&
        ClipboardRules::IsClipboardViewerIgnoreFormat(0, L"CF_CLIPBOARD_VIEWER_IGNORE");
}

} // namespace

int main() {
    if (!CheckApplicationMatching()) return 1;
    if (!CheckSearchParts()) return 2;
    if (!CheckPinKeys()) return 3;
    if (!CheckBoundedDecoding()) return 4;
    if (!CheckStripMarkup()) return 5;
    if (!CheckRegexBoundAndIgnoreAliases()) return 6;
    return 0;
}
