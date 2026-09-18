#include "ClipboardMonitor.h"
#include "ClipboardRules.h"
#include "Constants.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <utility>

#include <shellapi.h>
#include <shlobj.h>

namespace {

constexpr wchar_t kExcludeClipboardContentFromMonitorProcessing[] =
    L"ExcludeClipboardContentFromMonitorProcessing";
constexpr wchar_t kCanIncludeInClipboardHistory[] = L"CanIncludeInClipboardHistory";

class ClipboardGuard {
public:
    explicit ClipboardGuard(HWND owner) : m_open(::OpenClipboard(owner) != FALSE) {}

    ~ClipboardGuard() {
        if (m_open) {
            ::CloseClipboard();
        }
    }

    ClipboardGuard(const ClipboardGuard &) = delete;
    ClipboardGuard &operator=(const ClipboardGuard &) = delete;

    bool IsOpen() const noexcept { return m_open; }

private:
    bool m_open = false;
};

class GlobalLockGuard {
public:
    explicit GlobalLockGuard(HGLOBAL handle)
        : m_handle(handle), m_data(handle != nullptr ? ::GlobalLock(handle) : nullptr) {}

    ~GlobalLockGuard() {
        if (m_data != nullptr) {
            ::GlobalUnlock(m_handle);
        }
    }

    GlobalLockGuard(const GlobalLockGuard &) = delete;
    GlobalLockGuard &operator=(const GlobalLockGuard &) = delete;

    void *Data() const noexcept { return m_data; }

private:
    HGLOBAL m_handle = nullptr;
    void *m_data = nullptr;
};

class AllocatedGlobal {
public:
    AllocatedGlobal() = default;
    explicit AllocatedGlobal(HGLOBAL handle) : m_handle(handle) {}

    ~AllocatedGlobal() {
        if (m_handle != nullptr) {
            ::GlobalFree(m_handle);
        }
    }

    AllocatedGlobal(const AllocatedGlobal &) = delete;
    AllocatedGlobal &operator=(const AllocatedGlobal &) = delete;

    AllocatedGlobal(AllocatedGlobal &&other) noexcept : m_handle(other.Release()) {}

    AllocatedGlobal &operator=(AllocatedGlobal &&other) noexcept {
        if (this != &other) {
            if (m_handle != nullptr) {
                ::GlobalFree(m_handle);
            }
            m_handle = other.Release();
        }
        return *this;
    }

    HGLOBAL Get() const noexcept { return m_handle; }
    HGLOBAL Release() noexcept {
        const HGLOBAL result = m_handle;
        m_handle = nullptr;
        return result;
    }

private:
    HGLOBAL m_handle = nullptr;
};

std::optional<DWORD> ReadClipboardDword(const wchar_t *format_name) {
    const UINT format = RegisterClipboardFormatW(format_name);
    if (format == 0) {
        return std::nullopt;
    }

    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(format));
    if (data == nullptr || GlobalSize(data) < sizeof(DWORD)) {
        return std::nullopt;
    }

    const GlobalLockGuard lock(data);
    const auto *source = static_cast<const unsigned char *>(lock.Data());
    if (source == nullptr) {
        return std::nullopt;
    }

    DWORD value = 0;
    std::memcpy(&value, source, sizeof(value));
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

std::optional<std::vector<unsigned char>> CaptureBitmapAsDib(HBITMAP bitmap) {
    if (bitmap == nullptr) {
        return std::nullopt;
    }

    BITMAP source{};
    if (::GetObjectW(bitmap, sizeof(source), &source) != sizeof(source) ||
        source.bmWidth <= 0 || source.bmHeight <= 0) {
        return std::nullopt;
    }

    const std::uint64_t width = static_cast<std::uint64_t>(source.bmWidth);
    const std::uint64_t height = static_cast<std::uint64_t>(source.bmHeight);
    const std::uint64_t pixel_count = width * height;
    const std::uint64_t pixel_bytes = width * sizeof(DWORD) * height;
    const std::uint64_t total_bytes = sizeof(BITMAPINFOHEADER) + pixel_bytes;
    if (pixel_count == 0 || pixel_count > ClipboardRules::Limits::kMaximumImagePixels ||
        width > static_cast<std::uint64_t>(std::numeric_limits<LONG>::max()) ||
        height > static_cast<std::uint64_t>(std::numeric_limits<LONG>::max()) ||
        pixel_bytes > std::numeric_limits<DWORD>::max() ||
        total_bytes > ClipboardRules::Limits::kMaximumClipboardBytes ||
        total_bytes > std::numeric_limits<size_t>::max()) {
        return std::nullopt;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(width);
    info.bmiHeader.biHeight = static_cast<LONG>(height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = static_cast<DWORD>(pixel_bytes);

    std::vector<unsigned char> result(static_cast<size_t>(total_bytes));
    std::memcpy(result.data(), &info.bmiHeader, sizeof(info.bmiHeader));

    HDC dc = ::GetDC(nullptr);
    if (dc == nullptr) {
        return std::nullopt;
    }
    const int copied = ::GetDIBits(
        dc,
        bitmap,
        0,
        static_cast<UINT>(height),
        result.data() + sizeof(BITMAPINFOHEADER),
        &info,
        DIB_RGB_COLORS
    );
    ::ReleaseDC(nullptr, dc);
    if (copied != static_cast<int>(height)) {
        return std::nullopt;
    }
    return result;
}

class ClipboardFingerprint {
public:
    void Add(const std::wstring& name, UINT format, const unsigned char* bytes, size_t count) noexcept {
        AddBytes(reinterpret_cast<const unsigned char*>(name.data()), name.size() * sizeof(wchar_t));
        AddBytes(reinterpret_cast<const unsigned char*>(&format), sizeof(format));
        AddBytes(bytes, count);
    }

    std::wstring Finish() const {
        std::wstringstream stream;
        stream << std::hex << std::setw(16) << std::setfill(L'0') << m_hash;
        return stream.str();
    }

private:
    void AddBytes(const unsigned char* bytes, size_t count) noexcept {
        for (size_t index = 0; index < count; ++index) {
            m_hash ^= bytes[index];
            m_hash *= 1099511628211ULL;
        }
    }

    std::uint64_t m_hash = 1469598103934665603ULL;
};

bool IsRedundantBitmapFormat(UINT format, bool has_dib, bool has_dibv5) noexcept {
    return (format == CF_DIB && has_dibv5) ||
        (format == CF_BITMAP && (has_dib || has_dibv5));
}

} // namespace

ClipboardMonitor::ClipboardMonitor(AppSettings& settings, IgnoreLists ignored_lists)
    : m_settings(settings) {
    ReloadIgnoreLists(std::move(ignored_lists));
}

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
    m_expectedClipboardFingerprint.reset();
    m_owner = nullptr;
}

void ClipboardMonitor::ReloadIgnoreLists(IgnoreLists ignored_lists) {
    m_ignoredApps = std::move(ignored_lists[0]);
    m_ignoredFormats = std::move(ignored_lists[1]);
    m_ignoredRegexpPatterns.clear();
    for (const std::wstring& pattern : ignored_lists[2]) {
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
            return ClipboardRules::EqualInsensitive(format, configured);
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
    const std::wstring actual_path = ClipboardRules::NormalizePath(std::wstring(actual));
    const std::wstring configured_path = ClipboardRules::NormalizePath(std::wstring(configured));
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
    if (capacity == 0 || capacity > ClipboardRules::Limits::kMaximumClipboardCharacters + 1) {
        return {};
    }
    const GlobalLockGuard lock(data);
    const auto* text = static_cast<const wchar_t*>(lock.Data());
    if (text == nullptr) {
        return {};
    }
    size_t length = 0;
    while (length < capacity && text[length] != L'\0') {
        ++length;
    }
    std::wstring result;
    if (length < capacity && length <= ClipboardRules::Limits::kMaximumClipboardCharacters) {
        result.assign(text, length);
    }
    return result;
}

std::wstring ClipboardMonitor::ExtractClipboardAnsiText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_TEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    if (storage_bytes == 0 || storage_bytes > ClipboardRules::Limits::kMaximumClipboardCharacters + 1) {
        return {};
    }
    const GlobalLockGuard lock(data);
    const auto* source = static_cast<const char*>(lock.Data());
    if (source == nullptr) {
        return {};
    }
    size_t length = 0;
    while (length < storage_bytes && source[length] != '\0') {
        ++length;
    }
    std::wstring result;
    if (length > 0 && length <= ClipboardRules::Limits::kMaximumClipboardCharacters) {
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
    ClipboardFingerprint fingerprint;
    for (const ClipboardFormatData& item : data) {
        fingerprint.Add(item.name, item.format, item.bytes.data(), item.bytes.size());
    }
    return fingerprint.Finish();
}

std::optional<ClipboardSnapshot> ClipboardMonitor::CaptureClipboard() const {
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
        format_names.push_back(ClipboardRules::FormatName(format));
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

    ClipboardSnapshot capture;
    capture.application = application;
    capture.has_text = false;
    capture.has_image = false;
    capture.has_files = false;
    std::wstring file_preview;
    const bool has_dib = IsClipboardFormatAvailable(CF_DIB) != FALSE;
    const bool has_dibv5 = IsClipboardFormatAvailable(CF_DIBV5) != FALSE;
    for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
        const std::wstring name = ClipboardRules::FormatName(format);
        const bool is_text = ClipboardRules::IsTextFormat(format, name);
        const bool is_image = ClipboardRules::IsImageFormat(format, name);
        const bool is_files = format == CF_HDROP;
        const bool enabled = (is_text && m_settings.save_text) ||
            (is_image && m_settings.save_images) ||
            (is_files && m_settings.save_files);
        if (!enabled || IsRedundantBitmapFormat(format, has_dib, has_dibv5)) {
            continue;
        }

        ClipboardFormatData data;
        if (format == CF_BITMAP) {
            const auto dib = CaptureBitmapAsDib(static_cast<HBITMAP>(GetClipboardData(format)));
            if (!dib.has_value()) {
                continue;
            }
            data.name = L"CF_DIB";
            data.format = CF_DIB;
            data.bytes = std::move(*dib);
        } else {
            const HGLOBAL handle = static_cast<HGLOBAL>(GetClipboardData(format));
            if (handle == nullptr) {
                continue;
            }
            const SIZE_T bytes = GlobalSize(handle);
            if (bytes == 0 || bytes > ClipboardRules::Limits::kMaximumClipboardBytes) {
                continue;
            }
            const GlobalLockGuard lock(handle);
            const auto* source = static_cast<const unsigned char*>(lock.Data());
            if (source == nullptr) {
                continue;
            }
            data.name = name;
            data.format = format;
            data.bytes.assign(source, source + bytes);
        }
        if (is_text) {
            capture.has_text = true;
        }
        if (is_image) {
            capture.has_image = true;
        }
        if (is_files) {
            capture.has_files = true;
            file_preview = FilesPreview(static_cast<HGLOBAL>(GetClipboardData(format)));
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
    capture.title = ClipboardRules::MakeTitle(capture.preview, m_settings.show_special_symbols);
    if (capture.title.empty()) {
        capture.title = capture.has_image ? L"[图片]" : (capture.has_files ? L"[文件]" : L"[剪贴板项目]");
    }
    return capture;
}

void ClipboardMonitor::ReadClipboardAndSave() {
    std::optional<ClipboardSnapshot> capture;
    for (int attempt = 0; attempt < 10; ++attempt) {
        ClipboardGuard clipboard(m_owner);
        if (!clipboard.IsOpen()) {
            ::Sleep(10);
            continue;
        }
        capture = CaptureClipboard();
        break;
    }
    if (!capture.has_value()) {
        m_expectedClipboardFingerprint.reset();
        return;
    }

    if (m_expectedClipboardFingerprint.has_value()) {
        const bool was_written_by_this_process =
            capture->fingerprint == *m_expectedClipboardFingerprint;
        m_expectedClipboardFingerprint.reset();
        if (was_written_by_this_process) {
            return;
        }
    }

    if (m_saveCallback) {
        m_saveCallback(std::move(*capture));
    }
}

bool ClipboardMonitor::WriteClipboardItem(const ClipboardItem &item, bool remove_formatting) {
    if (item.data.empty()) {
        return false;
    }

    const bool has_plain_text = std::any_of(
        item.data.begin(),
        item.data.end(),
        [](const ClipboardFormatData& data) {
            return ClipboardRules::IsTextFormat(data.format, data.name);
        }
    );
    struct PendingFormat {
        UINT format = 0;
        AllocatedGlobal data;
    };

    std::vector<PendingFormat> pending;
    ClipboardFingerprint fingerprint;
    bool has_text = false;
    for (const ClipboardFormatData& data : item.data) {
        const bool is_text = ClipboardRules::IsTextFormat(data.format, data.name);
        const bool is_files = ClipboardRules::IsFilesFormat(data);
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

        AllocatedGlobal handle(::GlobalAlloc(GMEM_MOVEABLE, data.bytes.size()));
        if (handle.Get() == nullptr) {
            return false;
        }
        {
            const GlobalLockGuard lock(handle.Get());
            if (lock.Data() == nullptr) {
                return false;
            }
            std::memcpy(lock.Data(), data.bytes.data(), data.bytes.size());
        }

        fingerprint.Add(data.name, format, data.bytes.data(), data.bytes.size());
        pending.push_back(PendingFormat{format, std::move(handle)});
        has_text = has_text || is_text;
    }

    if (pending.empty() || (has_plain_text && !has_text)) {
        return false;
    }

    ClipboardGuard clipboard(m_owner);
    if (!clipboard.IsOpen() || ::EmptyClipboard() == FALSE) {
        return false;
    }
    for (PendingFormat &format : pending) {
        if (::SetClipboardData(format.format, format.data.Get()) == nullptr) {
            return false;
        }
        format.data.Release();
    }
    m_expectedClipboardFingerprint = fingerprint.Finish();
    return true;
}

bool ClipboardMonitor::ClearClipboard() {
    ClipboardGuard clipboard(m_owner);
    if (!clipboard.IsOpen() || ::EmptyClipboard() == FALSE) {
        return false;
    }
    m_expectedClipboardFingerprint.reset();
    return true;
}

void ClipboardMonitor::OnClipboardUpdate() {
    if (m_settings.ignore_events) {
        if (m_settings.ignore_only_next_event) {
            m_settings.ignore_events = false;
            m_settings.ignore_only_next_event = false;
        }
        return;
    }
    ReadClipboardAndSave();
}
