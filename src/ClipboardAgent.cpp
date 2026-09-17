#include "ClipboardAgent.h"
#include "Constants.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <iomanip>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <shellapi.h>
#include <shlobj.h>

namespace {

constexpr wchar_t kClipboardAgentWindowClass[] = L"maccy.ClipboardAgentWindow";
constexpr size_t kMaximumClipboardCharacters = 1024 * 1024;
constexpr size_t kMaximumClipboardBytes = 32 * 1024 * 1024;
constexpr wchar_t kExcludeClipboardContentFromMonitorProcessing[] =
    L"ExcludeClipboardContentFromMonitorProcessing";
constexpr wchar_t kCanIncludeInClipboardHistory[] = L"CanIncludeInClipboardHistory";

void RegisterClipboardAgentWindowClass() {
    WNDCLASSEXW window_class{sizeof(window_class)};
    window_class.lpfnWndProc = &ClipboardAgent::WindowProc;
    window_class.hInstance = ::GetModuleHandleW(nullptr);
    window_class.lpszClassName = kClipboardAgentWindowClass;
    if (::RegisterClassExW(&window_class) == 0 && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error("Unable to register clipboard agent window class");
    }
}

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
    if (pixel_count == 0 || pixel_count > 64ULL * 1024ULL * 1024ULL ||
        width > static_cast<std::uint64_t>(std::numeric_limits<LONG>::max()) ||
        height > static_cast<std::uint64_t>(std::numeric_limits<LONG>::max()) ||
        pixel_bytes > std::numeric_limits<DWORD>::max() ||
        total_bytes > kMaximumClipboardBytes ||
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

ClipboardAgentSettings ClipboardAgentSettings::FromAppSettings(const AppSettings &settings) {
    ClipboardAgentSettings result;
    result.respect_windows_clipboard_history_markers =
        settings.respect_windows_clipboard_history_markers;
    result.ignore_all_apps_except_listed = settings.ignore_all_apps_except_listed;
    result.ignore_events = settings.ignore_events;
    result.ignore_only_next_event = settings.ignore_only_next_event;
    result.save_files = settings.save_files;
    result.save_images = settings.save_images;
    result.save_text = settings.save_text;
    result.show_special_symbols = settings.show_special_symbols;
    return result;
}

ClipboardAgent::ClipboardAgent(
    SnapshotHandler on_snapshot,
    SettingsChangedHandler on_settings_changed
)
    : m_onSnapshot(std::move(on_snapshot)),
      m_onSettingsChanged(std::move(on_settings_changed)) {}

ClipboardAgent::~ClipboardAgent() {
    Stop();
}

void ClipboardAgent::Start(
    ClipboardAgentSettings settings,
    std::array<std::vector<std::wstring>, 3> ignored_lists
) {
    if (m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_readyMutex);
        m_ready = false;
        m_startupError = nullptr;
    }
    {
        std::lock_guard lock(m_commandMutex);
        m_commands.clear();
        m_initialSettings = std::move(settings);
        m_initialIgnoredLists = std::move(ignored_lists);
    }
    m_thread = std::jthread([this](std::stop_token stop_token) {
        ThreadMain(stop_token);
    });

    std::unique_lock lock(m_readyMutex);
    m_readyCondition.wait(lock, [this] { return m_ready; });
    if (m_startupError != nullptr) {
        const std::exception_ptr error = m_startupError;
        lock.unlock();
        Stop();
        std::rethrow_exception(error);
    }
    m_accepting.store(true, std::memory_order_release);
}

void ClipboardAgent::Stop() {
    if (!m_thread.joinable()) {
        return;
    }
    m_accepting.store(false, std::memory_order_release);
    m_thread.request_stop();
    if (const HWND window = m_workerWindow.load(std::memory_order_acquire);
        window != nullptr) {
        ::PostMessageW(window, AppConstants::kClipboardAgentShutdownMessage, 0, 0);
    }
    m_thread.join();
    m_workerWindow.store(nullptr, std::memory_order_release);
    {
        std::lock_guard lock(m_commandMutex);
        m_commands.clear();
    }
}

bool ClipboardAgent::SetSettings(ClipboardAgentSettings settings) {
    return Enqueue([this, settings = std::move(settings)]() mutable {
        ApplySettings(std::move(settings));
    });
}

bool ClipboardAgent::SetIgnoreLists(
    std::array<std::vector<std::wstring>, 3> ignored_lists
) {
    return Enqueue([this, ignored_lists = std::move(ignored_lists)]() mutable {
        ApplyIgnoreLists(std::move(ignored_lists));
    });
}

bool ClipboardAgent::WriteItem(
    ClipboardItem item,
    bool remove_formatting,
    OperationCallback callback
) {
    if (item.data.empty()) {
        return false;
    }
    return Enqueue([this, item = std::move(item), remove_formatting,
                    callback = std::move(callback)]() mutable {
        std::string error;
        const bool success = WriteClipboardItem(item, remove_formatting, error);
        if (callback) {
            callback(success, std::move(error));
        }
    });
}

bool ClipboardAgent::ClearClipboard(OperationCallback callback) {
    return Enqueue([this, callback = std::move(callback)]() mutable {
        std::string error;
        const bool success = ClearSystemClipboard(error);
        if (callback) {
            callback(success, std::move(error));
        }
    });
}

bool ClipboardAgent::Enqueue(Command command) {
    if (!command || !m_accepting.load(std::memory_order_acquire)) {
        return false;
    }
    std::lock_guard lock(m_commandMutex);
    if (!m_accepting.load(std::memory_order_relaxed) ||
        m_commands.size() >= kMaximumQueuedCommands) {
        return false;
    }
    const HWND window = m_workerWindow.load(std::memory_order_acquire);
    if (window == nullptr) {
        return false;
    }
    m_commands.push_back(std::move(command));
    if (::PostMessageW(window, AppConstants::kClipboardAgentCommandMessage, 0, 0) != FALSE) {
        return true;
    }
    m_commands.pop_back();
    return false;
}

void ClipboardAgent::ProcessCommands() {
    std::deque<Command> commands;
    {
        std::lock_guard lock(m_commandMutex);
        commands.swap(m_commands);
    }
    for (Command &command : commands) {
        if (command) {
            command();
        }
    }
}

void ClipboardAgent::ApplySettings(ClipboardAgentSettings settings) {
    m_settings = std::move(settings);
}

void ClipboardAgent::ApplyIgnoreLists(
    std::array<std::vector<std::wstring>, 3> ignored_lists
) {
    m_ignoredApps = std::move(ignored_lists[0]);
    m_ignoredFormats = std::move(ignored_lists[1]);
    m_ignoredRegexpPatterns.clear();
    for (const std::wstring &pattern : ignored_lists[2]) {
        try {
            m_ignoredRegexpPatterns.emplace_back(pattern);
        } catch (const std::regex_error&) {
            // Keep invalid user rules inert without disabling monitoring.
        }
    }
}

bool ClipboardAgent::ShouldIgnoreApplication(std::wstring_view application) const {
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

bool ClipboardAgent::ShouldIgnoreFormat(std::wstring_view format) const {
    return std::any_of(
        m_ignoredFormats.begin(),
        m_ignoredFormats.end(),
        [&format](const std::wstring& configured) {
            return EqualInsensitive(format, configured);
        }
    );
}

bool ClipboardAgent::ShouldIgnoreText(std::wstring_view text) const {
    for (const std::wregex& expression : m_ignoredRegexpPatterns) {
        if (std::regex_search(text.begin(), text.end(), expression)) {
            return true;
        }
    }
    return false;
}

std::wstring ClipboardAgent::GetSourceApplication() {
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

bool ClipboardAgent::MatchesApplication(std::wstring_view actual, std::wstring_view configured) {
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

std::wstring ClipboardAgent::ExtractClipboardText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_UNICODETEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    const SIZE_T capacity = storage_bytes / sizeof(wchar_t);
    if (capacity == 0 || capacity > kMaximumClipboardCharacters + 1) {
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
    if (length < capacity && length <= kMaximumClipboardCharacters) {
        result.assign(text, length);
    }
    return result;
}

std::wstring ClipboardAgent::ExtractClipboardAnsiText() {
    const HGLOBAL data = static_cast<HGLOBAL>(GetClipboardData(CF_TEXT));
    if (data == nullptr) {
        return {};
    }
    const SIZE_T storage_bytes = GlobalSize(data);
    if (storage_bytes == 0 || storage_bytes > kMaximumClipboardCharacters + 1) {
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
    return result;
}

std::wstring ClipboardAgent::FilesPreview(HGLOBAL data) {
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

std::wstring ClipboardAgent::HashSnapshot(const std::vector<ClipboardFormatData> &data) {
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

std::optional<ClipboardSnapshot> ClipboardAgent::CaptureClipboard() const {
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

    ClipboardSnapshot capture;
    capture.application = application;
    capture.has_text = false;
    capture.has_image = false;
    capture.has_files = false;
    std::wstring file_preview;
    const bool has_dib_format = IsClipboardFormatAvailable(CF_DIB) != FALSE ||
        IsClipboardFormatAvailable(CF_DIBV5) != FALSE;
    for (UINT format = 0; (format = EnumClipboardFormats(format)) != 0;) {
        const std::wstring name = FormatName(format);
        const bool is_text = IsTextFormat(format, name);
        const bool is_image = IsImageFormat(format, name);
        const bool is_files = format == CF_HDROP;
        const bool enabled = (is_text && m_settings.save_text) ||
            (is_image && m_settings.save_images) ||
            (is_files && m_settings.save_files);
        if (!enabled || (format == CF_BITMAP && has_dib_format)) {
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
            if (bytes == 0 || bytes > kMaximumClipboardBytes) {
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
    capture.fingerprint = HashSnapshot(capture.data);
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

bool ClipboardAgent::ReadClipboardSnapshot() {
    std::optional<ClipboardSnapshot> snapshot;
    for (int attempt = 0; attempt < 10; ++attempt) {
        ClipboardGuard clipboard(m_owner);
        if (!clipboard.IsOpen()) {
            ::Sleep(10);
            continue;
        }
        snapshot = CaptureClipboard();
        break;
    }
    if (!snapshot.has_value()) {
        m_expectedClipboardFingerprint.reset();
        return false;
    }

    if (m_expectedClipboardFingerprint.has_value()) {
        const bool was_written_by_this_process =
            snapshot->fingerprint == *m_expectedClipboardFingerprint;
        m_expectedClipboardFingerprint.reset();
        if (was_written_by_this_process) {
            return false;
        }
    }

    if (!m_onSnapshot || !m_onSnapshot(std::move(*snapshot))) {
        m_droppedSnapshots.fetch_add(1, std::memory_order_acq_rel);
        return false;
    }
    return true;
}

bool ClipboardAgent::WriteClipboardItem(
    const ClipboardItem &item,
    bool remove_formatting,
    std::string &error
) {
    if (item.data.empty()) {
        error = "剪贴板项目没有内容";
        return false;
    }

    const bool has_plain_text = std::any_of(
        item.data.begin(),
        item.data.end(),
        [](const ClipboardFormatData &data) {
            return data.format == CF_UNICODETEXT || data.format == CF_TEXT ||
                data.name == L"CF_UNICODETEXT" || data.name == L"CF_TEXT";
        }
    );
    struct PendingFormat {
        UINT format = 0;
        AllocatedGlobal data;
    };

    std::vector<PendingFormat> pending;
    std::vector<ClipboardFormatData> written_formats;
    bool has_text = false;
    for (const ClipboardFormatData &data : item.data) {
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

        AllocatedGlobal handle(::GlobalAlloc(GMEM_MOVEABLE, data.bytes.size()));
        if (handle.Get() == nullptr) {
            error = "无法分配剪贴板内存";
            return false;
        }
        {
            const GlobalLockGuard lock(handle.Get());
            if (lock.Data() == nullptr) {
                error = "无法锁定剪贴板内存";
                return false;
            }
            std::memcpy(lock.Data(), data.bytes.data(), data.bytes.size());
        }

        ClipboardFormatData written = data;
        written.format = format;
        written_formats.push_back(std::move(written));
        pending.push_back(PendingFormat{format, std::move(handle)});
        has_text = has_text || is_text;
    }

    if (pending.empty() || (has_plain_text && !has_text)) {
        error = "剪贴板项目没有可写入的格式";
        return false;
    }

    ClipboardGuard clipboard(m_owner);
    if (!clipboard.IsOpen()) {
        error = "无法打开系统剪贴板";
        return false;
    }
    if (::EmptyClipboard() == FALSE) {
        error = "无法清空系统剪贴板";
        return false;
    }
    for (PendingFormat &format : pending) {
        if (::SetClipboardData(format.format, format.data.Get()) == nullptr) {
            error = "无法写入系统剪贴板格式";
            return false;
        }
        format.data.Release();
    }
    m_expectedClipboardFingerprint = HashSnapshot(written_formats);
    return true;
}

bool ClipboardAgent::ClearSystemClipboard(std::string &error) {
    ClipboardGuard clipboard(m_owner);
    if (!clipboard.IsOpen()) {
        error = "无法打开系统剪贴板";
        return false;
    }
    if (::EmptyClipboard() == FALSE) {
        error = "无法清空系统剪贴板";
        return false;
    }
    m_expectedClipboardFingerprint.reset();
    return true;
}

void ClipboardAgent::NotifySettingsChanged() {
    if (m_onSettingsChanged) {
        m_onSettingsChanged(m_settings);
    }
}

void ClipboardAgent::HandleClipboardUpdate() {
    if (m_settings.ignore_events) {
        if (m_settings.ignore_only_next_event) {
            m_settings.ignore_events = false;
            m_settings.ignore_only_next_event = false;
            NotifySettingsChanged();
        }
        return;
    }
    try {
        ReadClipboardSnapshot();
    } catch (const std::exception &error) {
        ::OutputDebugStringA(error.what());
        ::OutputDebugStringA("\n");
    } catch (...) {
        ::OutputDebugStringA("Unhandled clipboard capture exception\n");
    }
}

void ClipboardAgent::HandleShutdown() {
    m_shuttingDown = true;
    if (m_clipboardListenerAdded && m_owner != nullptr) {
        ::RemoveClipboardFormatListener(m_owner);
        m_clipboardListenerAdded = false;
    }
    ProcessCommands();
    ::PostQuitMessage(0);
}

LRESULT CALLBACK ClipboardAgent::WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    auto *agent = reinterpret_cast<ClipboardAgent *>(::GetWindowLongPtrW(
        window,
        GWLP_USERDATA
    ));
    if (message == WM_NCCREATE) {
        const auto *create = reinterpret_cast<const CREATESTRUCTW *>(lParam);
        agent = static_cast<ClipboardAgent *>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(agent));
    }
    if (agent == nullptr) {
        return ::DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message) {
    case AppConstants::kClipboardAgentCommandMessage:
        agent->ProcessCommands();
        return 0;
    case AppConstants::kClipboardAgentShutdownMessage:
        agent->HandleShutdown();
        return 0;
    case WM_CLIPBOARDUPDATE:
        if (!agent->m_shuttingDown) {
            agent->HandleClipboardUpdate();
        }
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

void ClipboardAgent::ThreadMain(std::stop_token stop_token) {
    try {
        RegisterClipboardAgentWindowClass();
        const HWND window = ::CreateWindowExW(
            0,
            kClipboardAgentWindowClass,
            L"maccy clipboard agent",
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            nullptr,
            ::GetModuleHandleW(nullptr),
            this
        );
        if (window == nullptr) {
            throw std::runtime_error("Unable to create clipboard agent window");
        }
        m_owner = window;
        m_workerWindow.store(window, std::memory_order_release);
        m_settings = m_initialSettings;
        ApplyIgnoreLists(m_initialIgnoredLists);
        if (::AddClipboardFormatListener(window) == FALSE) {
            throw std::runtime_error("Unable to register clipboard listener");
        }
        m_clipboardListenerAdded = true;
        SignalReady();
        if (stop_token.stop_requested()) {
            ::PostMessageW(window, AppConstants::kClipboardAgentShutdownMessage, 0, 0);
        }

        MSG message{};
        while (::GetMessageW(&message, nullptr, 0, 0) > 0) {
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        if (m_clipboardListenerAdded) {
            ::RemoveClipboardFormatListener(window);
            m_clipboardListenerAdded = false;
        }
        m_expectedClipboardFingerprint.reset();
        m_owner = nullptr;
        ::DestroyWindow(window);
        m_workerWindow.store(nullptr, std::memory_order_release);
    } catch (...) {
        SignalStartupFailure(std::current_exception());
    }
}

void ClipboardAgent::SignalReady() {
    {
        std::lock_guard lock(m_readyMutex);
        m_ready = true;
    }
    m_readyCondition.notify_one();
}

void ClipboardAgent::SignalStartupFailure(std::exception_ptr error) {
    {
        std::lock_guard lock(m_readyMutex);
        m_startupError = std::move(error);
        m_ready = true;
    }
    m_readyCondition.notify_one();
}
