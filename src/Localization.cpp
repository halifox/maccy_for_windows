#include "Localization.h"

#include <boost/locale.hpp>

#include <locale>
#include <mutex>
#include <string>

namespace {

bool g_chinese = false;
std::locale g_locale = std::locale::classic();
std::once_flag g_initialize_once;

LANGID ResourceLanguage() noexcept {
    return g_chinese
        ? MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED)
        : MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
}

void InitializeLocale() noexcept {
    try {
        boost::locale::generator generator;
        g_locale = generator("");
        const auto &info = std::use_facet<boost::locale::info>(g_locale);
        g_chinese = info.language() == "zh";
    } catch (...) {
        const LANGID user_language = ::GetUserDefaultUILanguage();
        g_chinese = PRIMARYLANGID(user_language) == LANG_CHINESE;
        g_locale = std::locale::classic();
    }

    ::SetThreadUILanguage(ResourceLanguage());
}

void EnsureInitialized() noexcept {
    std::call_once(g_initialize_once, InitializeLocale);
    ::SetThreadUILanguage(ResourceLanguage());
}

} // namespace

namespace Localization {

void Initialize() noexcept {
    EnsureInitialized();
}

bool IsChinese() noexcept {
    EnsureInitialized();
    return g_chinese;
}

std::wstring FromUtf8(std::string_view value) {
    if (value.empty()) {
        return {};
    }

    EnsureInitialized();
    try {
        return boost::locale::conv::utf_to_utf<wchar_t>(std::string(value));
    } catch (const boost::locale::conv::conversion_error &) {
        // Keep the Win32 conversion as a last-resort fallback for malformed
        // server data so an update error can still be displayed.
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
    EnsureInitialized();
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
    EnsureInitialized();
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
