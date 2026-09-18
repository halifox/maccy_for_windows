#include "ClipboardRules.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <cstring>
#include <limits>

#include <shellapi.h>
#include <shlobj.h>

namespace ClipboardRules {
namespace {

struct SearchAccumulator {
    std::wstring unicode_text;
    std::wstring ansi_text;
    std::wstring rich_text;
    std::wstring paths;
};

} // namespace

const std::vector<std::wstring> &DefaultIgnoredFormats() {
    static const std::vector<std::wstring> values = {
        L"CF_CLIPBOARD_VIEWER_IGNORE",
    };
    return values;
}

std::wstring Lower(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (const wchar_t character : value) {
        result.push_back(static_cast<wchar_t>(::towlower(character)));
    }
    return result;
}

std::wstring TrimWhitespace(std::wstring value) {
    const auto is_space = [](wchar_t character) {
        return std::iswspace(character) != 0;
    };
    const auto first = std::find_if_not(value.begin(), value.end(), is_space);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    if (first >= last) {
        return {};
    }
    return std::wstring(first, last);
}

bool EqualInsensitive(std::wstring_view lhs, std::wstring_view rhs) {
    return Lower(lhs) == Lower(rhs);
}

std::wstring NormalizePath(std::wstring value) {
    value = Lower(TrimWhitespace(std::move(value)));
    while (!value.empty() && (value.back() == L'\\' || value.back() == L'/')) {
        value.pop_back();
    }
    return value;
}

std::wstring MakeTitle(std::wstring value, bool show_special_symbols) {
    value.resize(std::min<size_t>(value.size(), 1000));
    if (!show_special_symbols) {
        return TrimWhitespace(std::move(value));
    }

    size_t leading = 0;
    while (leading < value.size() && value[leading] == L' ') {
        value[leading++] = L'\x00b7';
    }
    size_t trailing = value.size();
    while (trailing > 0 && value[trailing - 1] == L' ') {
        value[--trailing] = L'\x00b7';
    }

    std::wstring result;
    result.reserve(value.size() + 8);
    for (const wchar_t character : value) {
        switch (character) {
        case L'\r':
            break;
        case L'\n':
            result += L'\x23ce';
            break;
        case L'\t':
            result += L'\x21e5';
            break;
        default:
            result += character;
            break;
        }
    }
    return result;
}

std::wstring StoredPreview(std::wstring value) {
    constexpr size_t kStoredPreviewCharacters = 4096;
    value.resize(std::min(value.size(), kStoredPreviewCharacters));
    return value;
}

std::wstring PreviewText(std::wstring_view text) {
    constexpr size_t kPreviewCharacters = 180;
    std::wstring preview;
    preview.reserve(std::min(text.size(), kPreviewCharacters + 3));
    for (const wchar_t character : text) {
        if (preview.size() >= kPreviewCharacters) {
            preview += L"...";
            break;
        }
        preview += (character < L' ' && character != L'\t') ? L' ' : character;
    }
    return preview;
}

std::wstring FormatName(UINT format) {
    switch (format) {
    case CF_UNICODETEXT:
        return L"CF_UNICODETEXT";
    case CF_TEXT:
        return L"CF_TEXT";
    case CF_HDROP:
        return L"CF_HDROP";
    case CF_DIB:
        return L"CF_DIB";
    case CF_DIBV5:
        return L"CF_DIBV5";
    case CF_BITMAP:
        return L"CF_BITMAP";
    default:
        break;
    }

    std::array<wchar_t, 256> name{};
    const int length = GetClipboardFormatNameW(format, name.data(), static_cast<int>(name.size()));
    if (length > 0) {
        return std::wstring(name.data(), static_cast<size_t>(length));
    }
    return L"FORMAT_" + std::to_wstring(format);
}

bool IsUnicodeTextFormat(const ClipboardFormatData &data) {
    return data.format == CF_UNICODETEXT || EqualInsensitive(data.name, L"CF_UNICODETEXT");
}

bool IsAnsiTextFormat(const ClipboardFormatData &data) {
    return data.format == CF_TEXT || EqualInsensitive(data.name, L"CF_TEXT");
}

bool IsRichTextFormat(const ClipboardFormatData &data) {
    return EqualInsensitive(data.name, L"HTML Format") ||
        EqualInsensitive(data.name, L"Rich Text Format");
}

bool IsFilesFormat(const ClipboardFormatData &data) {
    return data.format == CF_HDROP || EqualInsensitive(data.name, L"CF_HDROP");
}

bool IsTextFormat(UINT format, std::wstring_view name) {
    return format == CF_UNICODETEXT || format == CF_TEXT ||
        EqualInsensitive(name, L"HTML Format") || EqualInsensitive(name, L"Rich Text Format");
}

bool IsImageFormat(UINT format, std::wstring_view name) {
    return format == CF_DIB || format == CF_DIBV5 || format == CF_BITMAP ||
        EqualInsensitive(name, L"PNG") || EqualInsensitive(name, L"image/png") ||
        EqualInsensitive(name, L"JFIF") || EqualInsensitive(name, L"image/jpeg") ||
        EqualInsensitive(name, L"TIFF") || EqualInsensitive(name, L"image/tiff") ||
        EqualInsensitive(name, L"HEIC") || EqualInsensitive(name, L"image/heic");
}

bool IsEncodedImageName(std::wstring_view name) {
    constexpr std::wstring_view names[] = {
        L"PNG", L"image/png", L"JFIF", L"image/jpeg",
        L"TIFF", L"image/tiff", L"HEIC", L"image/heic",
    };
    return std::any_of(std::begin(names), std::end(names), [name](std::wstring_view expected) {
        return EqualInsensitive(name, expected);
    });
}

bool IsDib(const ClipboardFormatData &data) {
    return data.format == CF_DIBV5 || data.format == CF_DIB;
}

std::wstring DecodeUnicodeText(const std::vector<unsigned char> &bytes) {
    if (bytes.size() < sizeof(wchar_t)) {
        return {};
    }
    const size_t count = bytes.size() / sizeof(wchar_t);
    std::wstring result(count, L'\0');
    std::memcpy(result.data(), bytes.data(), count * sizeof(wchar_t));
    const size_t nul = result.find(L'\0');
    if (nul != std::wstring::npos) {
        result.resize(nul);
    }
    return result;
}

std::wstring DecodeAnsiText(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return {};
    }
    const size_t nul = std::find(bytes.begin(), bytes.end(), 0) - bytes.begin();
    if (nul == 0 || nul > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const int source_length = static_cast<int>(nul);
    const auto *source = reinterpret_cast<const char *>(bytes.data());
    const int length = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, source, source_length, nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, source, source_length, result.data(), length);
    return result;
}

std::wstring DecodeByteText(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return {};
    }
    const size_t nul = std::find(bytes.begin(), bytes.end(), 0) - bytes.begin();
    if (nul == 0 || nul > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return {};
    }
    const int source_length = static_cast<int>(nul);
    const auto *source = reinterpret_cast<const char *>(bytes.data());
    UINT code_page = CP_UTF8;
    DWORD flags = MB_ERR_INVALID_CHARS;
    int length = MultiByteToWideChar(code_page, flags, source, source_length, nullptr, 0);
    if (length <= 0) {
        code_page = CP_ACP;
        flags = MB_PRECOMPOSED;
        length = MultiByteToWideChar(code_page, flags, source, source_length, nullptr, 0);
    }
    if (length <= 0) {
        return {};
    }
    std::wstring result(static_cast<size_t>(length), L'\0');
    MultiByteToWideChar(code_page, flags, source, source_length, result.data(), length);
    return result;
}

std::wstring StripHtml(std::wstring value) {
    const size_t header_end = value.find(L"\r\n\r\n");
    if (header_end != std::wstring::npos) {
        value.erase(0, header_end + 4);
    }

    std::wstring result;
    result.reserve(value.size());
    bool in_tag = false;
    for (size_t index = 0; index < value.size(); ++index) {
        const wchar_t character = value[index];
        if (character == L'<') {
            in_tag = true;
            result.push_back(L' ');
            continue;
        }
        if (in_tag) {
            if (character == L'>') {
                in_tag = false;
            }
            continue;
        }
        if (character == L'&') {
            const size_t end = value.find(L';', index + 1);
            if (end != std::wstring::npos && end - index <= 16) {
                const std::wstring_view entity(value.data() + index + 1, end - index - 1);
                if (entity == L"nbsp") result += L' ';
                else if (entity == L"amp") result += L'&';
                else if (entity == L"lt") result += L'<';
                else if (entity == L"gt") result += L'>';
                else if (entity == L"quot") result += L'\"';
                else result.append(value, index, end - index + 1);
                index = end;
                continue;
            }
        }
        result.push_back(character);
    }
    return TrimWhitespace(std::move(result));
}

std::wstring StripRtf(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (size_t index = 0; index < value.size();) {
        const wchar_t character = value[index++];
        if (character == L'{' || character == L'}') {
            continue;
        }
        if (character != L'\\') {
            result.push_back(character);
            continue;
        }
        if (index >= value.size()) {
            break;
        }
        if (value[index] == L'\'') {
            index = std::min(index + 3, value.size());
            continue;
        }
        const size_t word_begin = index;
        while (index < value.size() && std::iswalpha(value[index])) {
            ++index;
        }
        const std::wstring_view word(value.data() + word_begin, index - word_begin);
        if (word == L"par" || word == L"line") {
            result.push_back(L'\n');
        }
        if (index < value.size() && (value[index] == L'-' || std::iswdigit(value[index]))) {
            ++index;
            while (index < value.size() && std::iswdigit(value[index])) {
                ++index;
            }
        }
        if (index < value.size() && value[index] == L' ') {
            ++index;
        }
    }
    return TrimWhitespace(std::move(result));
}

std::wstring ExtractDropPaths(const std::vector<unsigned char> &bytes) {
    if (bytes.size() < sizeof(DROPFILES)) {
        return {};
    }
    DROPFILES header{};
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.pFiles < sizeof(DROPFILES) || header.pFiles >= bytes.size()) {
        return {};
    }

    std::wstring result;
    size_t offset = header.pFiles;
    while (offset < bytes.size()) {
        std::wstring path;
        if (header.fWide) {
            while (offset + sizeof(wchar_t) <= bytes.size()) {
                wchar_t character = L'\0';
                std::memcpy(&character, bytes.data() + offset, sizeof(character));
                offset += sizeof(character);
                if (character == L'\0') {
                    break;
                }
                path.push_back(character);
            }
        } else {
            std::string ansi;
            while (offset < bytes.size() && bytes[offset] != 0) {
                ansi.push_back(static_cast<char>(bytes[offset++]));
            }
            if (offset < bytes.size()) {
                ++offset;
            }
            if (!ansi.empty()) {
                const int length = MultiByteToWideChar(
                    CP_ACP,
                    MB_PRECOMPOSED,
                    ansi.data(),
                    static_cast<int>(ansi.size()),
                    nullptr,
                    0
                );
                if (length > 0) {
                    path.resize(static_cast<size_t>(length));
                    MultiByteToWideChar(
                        CP_ACP,
                        MB_PRECOMPOSED,
                        ansi.data(),
                        static_cast<int>(ansi.size()),
                        path.data(),
                        length
                    );
                }
            }
        }
        if (path.empty()) {
            break;
        }
        if (!result.empty()) {
            result += L"; ";
        }
        result += path;
    }
    return result;
}

SearchParts SearchPartsFromFormats(const std::vector<ClipboardFormatData> &data) {
    SearchAccumulator accumulator;
    for (const ClipboardFormatData &format : data) {
        if (IsUnicodeTextFormat(format) && accumulator.unicode_text.empty()) {
            accumulator.unicode_text = DecodeUnicodeText(format.bytes);
        } else if (IsAnsiTextFormat(format) && accumulator.ansi_text.empty()) {
            accumulator.ansi_text = DecodeAnsiText(format.bytes);
        } else if (IsRichTextFormat(format) && accumulator.rich_text.empty()) {
            const std::wstring decoded = DecodeByteText(format.bytes);
            accumulator.rich_text = EqualInsensitive(format.name, L"HTML Format")
                ? StripHtml(decoded)
                : StripRtf(decoded);
        } else if (IsFilesFormat(format) && accumulator.paths.empty()) {
            accumulator.paths = ExtractDropPaths(format.bytes);
        }
    }

    SearchParts result;
    result.body = !accumulator.unicode_text.empty()
        ? std::move(accumulator.unicode_text)
        : (!accumulator.ansi_text.empty()
            ? std::move(accumulator.ansi_text)
            : std::move(accumulator.rich_text));
    result.paths = std::move(accumulator.paths);
    return result;
}

} // namespace ClipboardRules
