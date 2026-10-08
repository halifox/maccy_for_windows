#include "Localization.h"

#include <string>

namespace {

bool g_chinese = false;

LANGID ResourceLanguage() noexcept {
    return g_chinese
        ? MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
        : MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
}

} // namespace

namespace Localization {

void Initialize() noexcept {
    const LANGID user_language = ::GetUserDefaultUILanguage();
    g_chinese = PRIMARYLANGID(user_language) == LANG_CHINESE;

    ::SetThreadUILanguage(ResourceLanguage());
}

bool IsChinese() noexcept {
    return g_chinese;
}

std::wstring FromUtf8(std::string_view value) {
    if (value.empty()) {
        return {};
    }
    const int length = ::MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );
    if (length <= 0) {
        return std::wstring(value.begin(), value.end());
    }
    std::wstring result(static_cast<size_t>(length), L'\0');
    if (::MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            length
        ) != length) {
        return std::wstring(value.begin(), value.end());
    }
    return result;
}

std::wstring Text(UINT resource_id) {
    // Resource lookup is thread-local. Keep worker threads on the same UI
    // language as the main thread when they create translated clipboard text.
    ::SetThreadUILanguage(ResourceLanguage());
    wchar_t buffer[1024] = {};
    const int length = ::LoadStringW(
        ::GetModuleHandleW(nullptr),
        resource_id,
        buffer,
        ARRAYSIZE(buffer)
    );
    return length > 0 ? std::wstring(buffer, static_cast<size_t>(length)) : std::wstring{};
}

std::wstring Format(
    UINT resource_id,
    std::initializer_list<std::wstring_view> values
) {
    std::wstring result = Text(resource_id);
    size_t index = 0;
    for (const std::wstring_view value : values) {
        const std::wstring placeholder = L"{" + std::to_wstring(index++) + L"}";
        size_t position = 0;
        while ((position = result.find(placeholder, position)) != std::wstring::npos) {
            result.replace(position, placeholder.size(), value.data(), value.size());
            position += value.size();
        }
    }
    return result;
}

} // namespace Localization
