#include "ClipboardMonitor.h"
#include "ClipboardRules.h"
#include "Constants.h"
#include "Localization.h"
#include "Win32Resources.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlgdi.h>

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
    explicit ClipboardGuard(HWND owner) {
        if (::OpenClipboard(owner) != FALSE) {
            m_open = true;
            return;
        }
        m_open = ::OpenClipboard(nullptr) != FALSE;
    }

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

std::optional<DWORD> ReadClipboardDword(const wchar_t *format_name) {
    const UINT format = RegisterClipboardFormatW(format_name);
    if (format == 0) {
        return std::nullopt;
    }

    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(format));
    if (data == nullptr || GlobalSize(data) < sizeof(DWORD)) {
        return std::nullopt;
    }

    const ScopedGlobalLock lock(data);
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
        source.bmWidth <= 0 || source.bmHeight == 0) {
        return std::nullopt;
    }

    // Top-down DIBs report a negative bmHeight; size math needs the absolute value (M-19).
    const LONG absolute_height =
        source.bmHeight < 0 ? -source.bmHeight : source.bmHeight;
    const std::uint64_t width = static_cast<std::uint64_t>(source.bmWidth);
    const std::uint64_t height = static_cast<std::uint64_t>(absolute_height);
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

    CWindowDC dc(nullptr);
    if (dc.m_hDC == nullptr) {
        return std::nullopt;
    }
    const int copied = ::GetDIBits(
        dc.m_hDC,
        bitmap,
        0,
        static_cast<UINT>(height),
        result.data() + sizeof(BITMAPINFOHEADER),
        &info,
        DIB_RGB_COLORS
    );
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
    m_selfWriteSequence.reset();
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
            return false;
        }
        return std::none_of(
            m_ignoredApps.begin(),
            m_ignoredApps.end(),
            [&application](const std::wstring& configured) {
                return ClipboardRules::MatchesApplication(application, configured);
            }
        );
    }
    return std::any_of(
        m_ignoredApps.begin(),
        m_ignoredApps.end(),
        [&application](const std::wstring& configured) {
            return ClipboardRules::MatchesApplication(application, configured);
        }
    );
}

bool ClipboardMonitor::ShouldIgnoreFormat(UINT format, std::wstring_view name) const {
    return std::any_of(
        m_ignoredFormats.begin(),
        m_ignoredFormats.end(),
        [format, name](const std::wstring &configured) {
            if (ClipboardRules::IsClipboardViewerIgnoreFormat(format, name) &&
                ClipboardRules::IsClipboardViewerIgnoreFormat(0, configured)) {
                return true;
            }
            return ClipboardRules::MatchesIgnoredFormatName(name, configured);
        }
    );
}

bool ClipboardMonitor::ShouldIgnoreText(std::wstring_view text) const {
    for (const std::wregex& expression : m_ignoredRegexpPatterns) {
        if (ClipboardRules::RegexSearchBounded(expression, text)) {
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
    const ScopedGlobalLock lock(data);
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
    const ScopedGlobalLock lock(data);
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
        constexpr size_t kStoredPreviewCharacters = 4096;
        const size_t separator = result.empty() ? 0 : 2;
        if (result.size() + separator + path.size() > kStoredPreviewCharacters) {
            break;
        }
        if (!result.empty()) {
            result += L"; ";
        }
        result += path;
    }
    return ClipboardRules::StoredPreview(std::move(result));
}

std::wstring ClipboardMonitor::HashCapture(const std::vector<ClipboardFormatData>& data) {
    std::vector<const ClipboardFormatData *> ordered;
    ordered.reserve(data.size());
    for (const ClipboardFormatData &item : data) {
        ordered.push_back(&item);
    }
    std::sort(ordered.begin(), ordered.end(), [](const ClipboardFormatData *lhs, const ClipboardFormatData *rhs) {
        if (lhs->format != rhs->format) {
            return lhs->format < rhs->format;
        }
        return lhs->name < rhs->name;
    });

    ClipboardFingerprint fingerprint;
    for (const ClipboardFormatData *item : ordered) {
        fingerprint.Add(item->name, item->format, item->bytes.data(), item->bytes.size());
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

    ClipboardSnapshot capture;
    capture.application = application;
    capture.has_text = false;
    capture.has_image = false;
    capture.has_files = false;
    std::wstring file_preview;
    std::vector<ClipboardFormatData> ignore_probe;
    const bool has_dib = IsClipboardFormatAvailable(CF_DIB) != FALSE;
    const bool has_dibv5 = IsClipboardFormatAvailable(CF_DIBV5) != FALSE;
    for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
        const std::wstring name = ClipboardRules::FormatName(format);
        if (ShouldIgnoreFormat(format, name)) {
            // Clipboard Viewer Ignore means "skip this entire update" by convention.
            if (ClipboardRules::IsClipboardViewerIgnoreFormat(format, name)) {
                return std::nullopt;
            }
            // Other ignored formats: omit that format only (M-20).
            continue;
        }

        const bool is_text = ClipboardRules::IsTextFormat(format, name);
        const bool is_image = ClipboardRules::IsImageFormat(format, name);
        const bool is_files = format == CF_HDROP;
        const bool enabled = (is_text && m_settings.save_text) ||
            (is_image && m_settings.save_images) ||
            (is_files && m_settings.save_files);

        ClipboardFormatData data;
        if (format == CF_BITMAP) {
            if (!enabled || IsRedundantBitmapFormat(format, has_dib, has_dibv5)) {
                continue;
            }
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
            const ScopedGlobalLock lock(handle);
            const auto* source = static_cast<const unsigned char*>(lock.Data());
            if (source == nullptr) {
                continue;
            }
            data.name = name;
            data.format = format;
            data.bytes.assign(source, source + bytes);
        }

        // Always probe text-like formats for regex ignore, even when save_text is off (M-25).
        if (is_text || ClipboardRules::IsRichTextFormat(data) ||
            ClipboardRules::IsUnicodeTextFormat(data) ||
            ClipboardRules::IsAnsiTextFormat(data)) {
            ignore_probe.push_back(data);
        }

        if (!enabled || IsRedundantBitmapFormat(format, has_dib, has_dibv5)) {
            continue;
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

    const ClipboardRules::SearchParts ignore_parts =
        ClipboardRules::SearchPartsFromFormats(ignore_probe);
    if (!ignore_parts.body.empty() && ShouldIgnoreText(ignore_parts.body)) {
        return std::nullopt;
    }
    const std::wstring text = ignore_parts.body;

    if (capture.data.empty()) {
        return std::nullopt;
    }
    capture.fingerprint = HashCapture(capture.data);
    capture.preview = !text.empty() ? text : file_preview;
    if (capture.preview.empty() && capture.has_image) {
        capture.preview = Localization::Text(IDS_CONTENT_IMAGE);
    }
    if (capture.preview.empty() && capture.has_files) {
        capture.preview = Localization::Text(IDS_CONTENT_FILE);
    }
    capture.title = ClipboardRules::MakeTitle(capture.preview, m_settings.show_special_symbols);
    if (capture.title.empty()) {
        capture.title = capture.has_image
            ? Localization::Text(IDS_CONTENT_IMAGE)
            : (capture.has_files
                ? Localization::Text(IDS_CONTENT_FILE)
                : Localization::Text(IDS_CONTENT_CLIPBOARD_ITEM));
    }
    return capture;
}

void ClipboardMonitor::ReadClipboardAndSave() {
    const DWORD sequence = ::GetClipboardSequenceNumber();
    if (m_selfWriteSequence.has_value()) {
        if (sequence == *m_selfWriteSequence) {
            // Our own write's clipboard update — drop the echo (H-01 / H-08).
            m_selfWriteSequence.reset();
            return;
        }
        // Clipboard advanced past our write without an exact match observation.
        m_selfWriteSequence.reset();
    }

    std::optional<ClipboardSnapshot> capture;
    bool opened = false;
    for (int attempt = 0; attempt < 10; ++attempt) {
        ClipboardGuard clipboard(m_owner);
        if (!clipboard.IsOpen()) {
            ::Sleep(10);
            continue;
        }
        opened = true;
        capture = CaptureClipboard();
        break;
    }
    // Keep going only when open succeeded; leave self-write state alone on busy
    // open failures so a later update can still suppress an echo (H-02).
    if (!opened) {
        return;
    }
    if (!capture.has_value()) {
        return;
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
        UniqueGlobal data;
    };

    std::vector<PendingFormat> pending;
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

        UniqueGlobal handle(::GlobalAlloc(GMEM_MOVEABLE, data.bytes.size()));
        if (handle.Get() == nullptr) {
            return false;
        }
        {
            const ScopedGlobalLock lock(handle.Get());
            if (lock.Data() == nullptr) {
                return false;
            }
            std::memcpy(lock.Data(), data.bytes.data(), data.bytes.size());
        }

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
            // Avoid leaving a partial clipboard after EmptyClipboard (H-03).
            ::EmptyClipboard();
            m_selfWriteSequence.reset();
            return false;
        }
        format.data.Release();
    }
    m_selfWriteSequence = ::GetClipboardSequenceNumber();
    return true;
}

bool ClipboardMonitor::ClearClipboard() {
    ClipboardGuard clipboard(m_owner);
    if (!clipboard.IsOpen() || ::EmptyClipboard() == FALSE) {
        return false;
    }
    m_selfWriteSequence.reset();
    return true;
}

void ClipboardMonitor::OnClipboardUpdate() {
    // One-shot skip is independent of continuous pause (ignore_events).
    if (m_settings.ignore_only_next_event) {
        m_settings.ignore_only_next_event = false;
        return;
    }
    if (m_settings.ignore_events) {
        return;
    }
    ReadClipboardAndSave();
}
