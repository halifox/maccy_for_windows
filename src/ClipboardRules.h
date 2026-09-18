#pragma once

#include "ClipboardData.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ClipboardRules {

namespace Limits {
    constexpr size_t kMaximumFormats = 4096;
    constexpr size_t kMaximumFormatNameBytes = 1024 * 1024;
    constexpr std::uint64_t kMaximumPayloadBytes = 256ULL * 1024ULL * 1024ULL;
    constexpr size_t kMaximumClipboardCharacters = 1024 * 1024;
    constexpr size_t kMaximumClipboardBytes = 32 * 1024 * 1024;
    constexpr std::uint64_t kMaximumImagePixels = 64ULL * 1024ULL * 1024ULL;
    constexpr size_t kMaximumPreviewTextCharacters = 4ULL * 1024ULL * 1024ULL;
}

const std::vector<std::wstring> &DefaultIgnoredFormats();

std::wstring Lower(std::wstring_view value);
std::wstring TrimWhitespace(std::wstring value);
bool EqualInsensitive(std::wstring_view lhs, std::wstring_view rhs);
std::wstring NormalizePath(std::wstring value);

std::wstring MakeTitle(std::wstring value, bool show_special_symbols);
std::wstring StoredPreview(std::wstring value);
std::wstring PreviewText(std::wstring_view value);

std::wstring FormatName(UINT format);
bool IsUnicodeTextFormat(const ClipboardFormatData &data);
bool IsAnsiTextFormat(const ClipboardFormatData &data);
bool IsRichTextFormat(const ClipboardFormatData &data);
bool IsFilesFormat(const ClipboardFormatData &data);
bool IsTextFormat(UINT format, std::wstring_view name);
bool IsImageFormat(UINT format, std::wstring_view name);
bool IsEncodedImageName(std::wstring_view name);
bool IsDib(const ClipboardFormatData &data);

std::wstring DecodeUnicodeText(const std::vector<unsigned char> &bytes);
std::wstring DecodeAnsiText(const std::vector<unsigned char> &bytes);
std::wstring DecodeByteText(const std::vector<unsigned char> &bytes);
std::wstring StripHtml(std::wstring value);
std::wstring StripRtf(std::wstring_view value);
std::wstring ExtractDropPaths(const std::vector<unsigned char> &bytes);

struct SearchParts {
    std::wstring body;
    std::wstring paths;
};

SearchParts SearchPartsFromFormats(const std::vector<ClipboardFormatData> &data);

} // namespace ClipboardRules
