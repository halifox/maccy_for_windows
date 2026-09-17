#include "ClipboardMonitor.h"
#include "Constants.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <regex>
#include <sstream>

#include <shellapi.h>
#include <shlobj.h>

namespace {

constexpr size_t kMaximumClipboardCharacters = 1024 * 1024;
constexpr size_t kMaximumClipboardBytes = 32 * 1024 * 1024;
constexpr wchar_t kExcludeClipboardContentFromMonitorProcessing[] =
    L"ExcludeClipboardContentFromMonitorProcessing";
constexpr wchar_t kCanIncludeInClipboardHistory[] = L"CanIncludeInClipboardHistory";

std::wstring Lower(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (const wchar_t character : value) {
        result.push_back(static_cast<wchar_t>(::towlower(character)));
    }
    return result;
}

std::wstring Trim(std::wstring value) {
    const auto is_space = [](wchar_t character) { return ::iswspace(character) != 0; };
    const auto first = std::find_if_not(value.begin(), value.end(), is_space);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    if (first >= last) {
        return {};
    }
    return std::wstring(first, last);
}

std::wstring NormalizePath(std::wstring value) {
    value = Lower(Trim(std::move(value)));
    while (!value.empty() && (value.back() == L'\\' || value.back() == L'/')) {
        value.pop_back();
    }
    return value;
}

bool EqualInsensitive(std::wstring_view lhs, std::wstring_view rhs) {
    return Lower(lhs) == Lower(rhs);
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

std::optional<DWORD> ReadClipboardDword(const wchar_t *format_name) {
    const UINT format = RegisterClipboardFormatW(format_name);
    if (format == 0) {
        return std::nullopt;
    }

    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(format));
    if (data == nullptr || GlobalSize(data) < sizeof(DWORD)) {
        return std::nullopt;
    }

    const auto *source = static_cast<const unsigned char *>(GlobalLock(data));
    if (source == nullptr) {
        return std::nullopt;
    }

    DWORD value = 0;
    std::memcpy(&value, source, sizeof(value));
    GlobalUnlock(data);
    return value;
}

bool IsExcludedByWindowsClipboardHistoryMarker() {
    const UINT exclude_format = RegisterClipboardFormatW(kExcludeClipboardContentFromMonitorProcessing);
    if (exclude_format != 0 && IsClipboardFormatAvailable(exclude_format)) {
        return true;
    }

    const auto can_include = ReadClipboardDword(kCanIncludeInClipboardHistory);
    return can_include.has_value() && *can_include == 0;
}

bool IsHtmlOrRtf(std::wstring_view name) {
    return EqualInsensitive(name, L"HTML Format") || EqualInsensitive(name, L"Rich Text Format");
}

bool IsImageFormat(UINT format, std::wstring_view name) {
    return format == CF_DIB || format == CF_DIBV5 || format == CF_BITMAP ||
        EqualInsensitive(name, L"PNG") || EqualInsensitive(name, L"image/png") ||
        EqualInsensitive(name, L"JFIF") || EqualInsensitive(name, L"image/jpeg") ||
        EqualInsensitive(name, L"TIFF") || EqualInsensitive(name, L"image/tiff") ||
        EqualInsensitive(name, L"HEIC") || EqualInsensitive(name, L"image/heic");
}

bool IsTextFormat(UINT format, std::wstring_view name) {
    return format == CF_UNICODETEXT || format == CF_TEXT || IsHtmlOrRtf(name);
}

std::wstring MakeTitle(std::wstring value, bool show_special_symbols) {
    value.resize(std::min<size_t>(value.size(), 1000));
    if (!show_special_symbols) {
        return Trim(std::move(value));
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

} // namespace

ClipboardMonitor::ClipboardMonitor(Database& database, AppSettings& settings)
    : m_database(database), m_settings(settings) {}

ClipboardMonitor::~ClipboardMonitor() {
    Shutdown();
}

bool ClipboardMonitor::Initialize(HWND owner) {
    m_owner = owner;
    m_clipboardListenerAdded = AddClipboardFormatListener(owner) == TRUE;
    return m_clipboardListenerAdded;
}

void ClipboardMonitor::Shutdown() {
    if (m_clipboardListenerAdded && m_owner != nullptr) {
        RemoveClipboardFormatListener(m_owner);
        m_clipboardListenerAdded = false;
    }
}

void ClipboardMonitor::ReloadIgnoreLists() {
    m_ignoredApps = m_database.GetList(DatabaseList::IgnoredApplications);
    m_ignoredFormats = m_database.GetList(DatabaseList::IgnoredFormats);
    m_ignoredRegexpPatterns.clear();
    for (const std::wstring& pattern : m_database.GetList(DatabaseList::IgnoredRegexps)) {
        try {
            m_ignoredRegexpPatterns.emplace_back(pattern);
        } catch (const std::regex_error&) {
            // Keep invalid user rules inert without disabling monitoring.
        }
    }
}

bool ClipboardMonitor::ShouldIgnoreApplication(std::wstring_view application) const {
    if (m_settings.ignore_all_apps_except_listed) {
        if (application.empty()) {
            return true;
        }
        return std::none_of(
            m_ignoredApps.begin(),
            m_ignoredApps.end(),
            [&application](const std::wstring& configured) {
                return MatchesApplication(application, configured);
            }
        );
    }
    return std::any_of(
        m_ignoredApps.begin(),
        m_ignoredApps.end(),
        [&application](const std::wstring& configured) {
            return MatchesApplication(application, configured);
        }
    );
}

bool ClipboardMonitor::ShouldIgnoreFormat(std::wstring_view format) const {
    return std::any_of(
        m_ignoredFormats.begin(),
        m_ignoredFormats.end(),
        [&format](const std::wstring& configured) {
            return EqualInsensitive(format, configured);
        }
    );
}

bool ClipboardMonitor::ShouldIgnoreText(std::wstring_view text) const {
    for (const std::wregex& expression : m_ignoredRegexpPatterns) {
        if (std::regex_search(text.begin(), text.end(), expression)) {
            return true;
        }
    }
    return false;
}

std::wstring ClipboardMonitor::GetSourceApplication() {
    HWND owner = GetClipboardOwner();
    if (owner == nullptr) {
        owner = GetOpenClipboardWindow();
    }
    if (owner == nullptr) {
        return {};
    }
    DWORD process_id = 0;
    GetWindowThreadProcessId(owner, &process_id);
    if (process_id == 0) {
        return {};
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, process_id);
    if (process == nullptr) {
        return {};
    }
    std::array<wchar_t, 32768> path{};
    DWORD length = static_cast<DWORD>(path.size());
    const BOOL ok = QueryFullProcessImageNameW(process, 0, path.data(), &length);
    CloseHandle(process);
    return ok ? std::wstring(path.data(), length) : std::wstring{};
}

bool ClipboardMonitor::MatchesApplication(std::wstring_view actual, std::wstring_view configured) {
    const std::wstring actual_path = NormalizePath(std::wstring(actual));
    const std::wstring configured_path = NormalizePath(std::wstring(configured));
    if (actual_path.empty() || configured_path.empty()) {
        return false;
    }
    if (actual_path == configured_path) {
        return true;
    }
    const size_t separator = configured_path.find_last_of(L"\\/");
    return separator == std::wstring::npos &&
        actual_path.substr(actual_path.find_last_of(L"\\/") + 1) == configured_path;
}

std::wstring ClipboardMonitor::ExtractClipboardText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_UNICODETEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    const SIZE_T capacity = storage_bytes / sizeof(wchar_t);
    if (capacity == 0 || capacity > kMaximumClipboardCharacters + 1) {
        return {};
    }
    const auto* text = static_cast<const wchar_t*>(GlobalLock(data));
    if (text == nullptr) {
        return {};
    }
    size_t length = 0;
    while (length < capacity && text[length] != L'\0') {
        ++length;
    }
    std::wstring result;
    if (length < capacity && length <= kMaximumClipboardCharacters) {
        result.assign(text, length);
    }
    GlobalUnlock(data);
    return result;
}

std::wstring ClipboardMonitor::ExtractClipboardAnsiText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_TEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    if (storage_bytes == 0 || storage_bytes > kMaximumClipboardCharacters + 1) {
        return {};
    }
    const auto* source = static_cast<const char*>(GlobalLock(data));
    if (source == nullptr) {
        return {};
    }
    size_t length = 0;
    while (length < storage_bytes && source[length] != '\0') {
        ++length;
    }
    std::wstring result;
    if (length > 0 && length <= kMaximumClipboardCharacters) {
        const int wide_length = MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED,
            source,
            static_cast<int>(length),
            nullptr,
            0
        );
        if (wide_length > 0) {
            result.resize(static_cast<size_t>(wide_length));
            MultiByteToWideChar(
                CP_ACP,
                MB_PRECOMPOSED,
                source,
                static_cast<int>(length),
                result.data(),
                wide_length
            );
        }
    }
    GlobalUnlock(data);
    return result;
}

std::wstring ClipboardMonitor::FilesPreview(HGLOBAL data) {
    if (data == nullptr) {
        return {};
    }
    const HDROP drop = static_cast<HDROP>(data);
    const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    std::wstring result;
    for (UINT index = 0; index < count; ++index) {
        const UINT length = DragQueryFileW(drop, index, nullptr, 0);
        std::wstring path(static_cast<size_t>(length) + 1, L'\0');
        DragQueryFileW(drop, index, path.data(), length + 1);
        path.resize(length);
        if (!result.empty()) {
            result += L"; ";
        }
        result += path;
    }
    return result;
}

std::wstring ClipboardMonitor::HashCapture(const std::vector<ClipboardFormatData>& data) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto add = [&hash](const unsigned char* bytes, size_t count) {
        for (size_t index = 0; index < count; ++index) {
            hash ^= bytes[index];
            hash *= 1099511628211ULL;
        }
    };
    for (const ClipboardFormatData& item : data) {
        add(reinterpret_cast<const unsigned char*>(item.name.data()), item.name.size() * sizeof(wchar_t));
        add(reinterpret_cast<const unsigned char*>(&item.format), sizeof(item.format));
        add(item.bytes.data(), item.bytes.size());
    }
    std::wstringstream stream;
    stream << std::hex << std::setw(16) << std::setfill(L'0') << hash;
    return stream.str();
}

std::optional<ClipboardCapture> ClipboardMonitor::CaptureClipboard() const {
    if (m_settings.respect_windows_clipboard_history_markers &&
        IsExcludedByWindowsClipboardHistoryMarker()) {
        return std::nullopt;
    }

    const std::wstring application = GetSourceApplication();
    if (ShouldIgnoreApplication(application)) {
        return std::nullopt;
    }

    std::wstring text;
    std::vector<std::wstring> format_names;
    for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
        format_names.push_back(FormatName(format));
    }
    if (std::any_of(
        format_names.begin(),
        format_names.end(),
        [this](const std::wstring& name) { return ShouldIgnoreFormat(name); }
    )) {
        return std::nullopt;
    }

    if (m_settings.save_text && IsClipboardFormatAvailable(CF_UNICODETEXT)) {
        text = ExtractClipboardText();
        if (text.empty() && IsClipboardFormatAvailable(CF_TEXT)) {
            text = ExtractClipboardAnsiText();
        }
    } else if (IsClipboardFormatAvailable(CF_UNICODETEXT) ||
               IsClipboardFormatAvailable(CF_TEXT)) {
        // Regex ignore rules still apply when text storage is disabled.
        text = IsClipboardFormatAvailable(CF_UNICODETEXT)
            ? ExtractClipboardText()
            : ExtractClipboardAnsiText();
    }
    if (!text.empty() && ShouldIgnoreText(text)) {
        return std::nullopt;
    }

    ClipboardCapture capture;
    capture.application = application;
    capture.has_text = false;
    capture.has_image = false;
    capture.has_files = false;
    std::wstring file_preview;
    for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
        const std::wstring name = FormatName(format);
        const bool is_text = IsTextFormat(format, name);
        const bool is_image = IsImageFormat(format, name);
        const bool is_files = format == CF_HDROP;
        const bool enabled = (is_text && m_settings.save_text) ||
            (is_image && m_settings.save_images) ||
            (is_files && m_settings.save_files);
        if (!enabled || format == CF_BITMAP) {
            continue;
        }

        const HGLOBAL handle = static_cast<HGLOBAL>(GetClipboardData(format));
        if (handle == nullptr) {
            continue;
        }
        const SIZE_T bytes = GlobalSize(handle);
        if (bytes == 0 || bytes > kMaximumClipboardBytes) {
            continue;
        }
        const auto* source = static_cast<const unsigned char*>(GlobalLock(handle));
        if (source == nullptr) {
            continue;
        }
        ClipboardFormatData data;
        data.name = name;
        data.format = format;
        data.bytes.assign(source, source + bytes);
        GlobalUnlock(handle);
        if (is_text) {
            capture.has_text = true;
        }
        if (is_image) {
            capture.has_image = true;
        }
        if (is_files) {
            capture.has_files = true;
            file_preview = FilesPreview(handle);
        }
        capture.data.push_back(std::move(data));
    }

    if (capture.data.empty()) {
        return std::nullopt;
    }
    capture.fingerprint = HashCapture(capture.data);
    capture.preview = !text.empty() ? text : file_preview;
    if (capture.preview.empty() && capture.has_image) {
        capture.preview = L"[图片]";
    }
    if (capture.preview.empty() && capture.has_files) {
        capture.preview = L"[文件]";
    }
    capture.title = MakeTitle(capture.preview, m_settings.show_special_symbols);
    if (capture.title.empty()) {
        capture.title = capture.has_image ? L"[图片]" : (capture.has_files ? L"[文件]" : L"[剪贴板项目]");
    }
    return capture;
}

bool ClipboardMonitor::ReadClipboardAndSave() {
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (!::OpenClipboard(m_owner)) {
            Sleep(5);
            continue;
        }
        const auto capture = CaptureClipboard();
        CloseClipboard();
        if (!capture) {
            return false;
        }
        m_database.SaveClipboard(*capture, m_settings.history_size);
        return true;
    }
    return false;
}

bool ClipboardMonitor::WriteClipboardItem(const ClipboardItem &item, bool remove_formatting) {
    if (item.data.empty()) {
        return false;
    }
    if (!::OpenClipboard(m_owner)) {
        return false;
    }
    bool success = EmptyClipboard() != FALSE;
    if (success) {
        m_skipNextClipboardEvent = true;
    }
    bool has_text = false;
    const bool has_plain_text = std::any_of(
        item.data.begin(),
        item.data.end(),
        [](const ClipboardFormatData& data) {
            return data.format == CF_UNICODETEXT || data.format == CF_TEXT ||
                data.name == L"CF_UNICODETEXT" || data.name == L"CF_TEXT";
        }
    );
    if (success) {
        for (const ClipboardFormatData& data : item.data) {
            const bool is_text = data.format == CF_UNICODETEXT || data.format == CF_TEXT ||
                data.name == L"CF_UNICODETEXT" || data.name == L"CF_TEXT";
            const bool is_files = data.format == CF_HDROP || data.name == L"CF_HDROP";
            if (remove_formatting && has_plain_text && !is_text && !is_files) {
                continue;
            }
            UINT format = data.format;
            if (format >= 0xC000 || format == 0) {
                format = RegisterClipboardFormatW(data.name.c_str());
            }
            if (format == 0 || data.bytes.empty()) {
                continue;
            }
            HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, data.bytes.size());
            if (handle == nullptr) {
                success = false;
                break;
            }
            void* destination = GlobalLock(handle);
            if (destination == nullptr) {
                GlobalFree(handle);
                success = false;
                break;
            }
            std::memcpy(destination, data.bytes.data(), data.bytes.size());
            GlobalUnlock(handle);
            if (SetClipboardData(format, handle) == nullptr) {
                GlobalFree(handle);
                success = false;
                break;
            }
            has_text = has_text || is_text;
        }
    }
    CloseClipboard();
    if (success && has_plain_text && !has_text) {
        return false;
    }
    return success;
}

bool ClipboardMonitor::OnClipboardUpdate() {
    if (m_skipNextClipboardEvent) {
        m_skipNextClipboardEvent = false;
        return false;
    }

    try {
        if (m_settings.ignore_events) {
            if (m_settings.ignore_only_next_event) {
                m_settings.ignore_events = false;
                m_settings.ignore_only_next_event = false;
                m_settings.Save(m_database);
            }
            return false;
        }
        return ReadClipboardAndSave();
    } catch (const std::exception& error) {
        OutputDebugStringA(error.what());
        OutputDebugStringA("\n");
        return false;
    }
}
