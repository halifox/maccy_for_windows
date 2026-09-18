#include "UpdateChecker.h"

#include "Constants.h"

#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace {

constexpr wchar_t kGitHubHost[] = L"api.github.com";
constexpr wchar_t kGitHubLatestReleasePath[] =
    L"/repos/halifox/maccy_for_windows/releases/latest";
constexpr wchar_t kGitHubReleasePage[] =
    L"https://github.com/halifox/maccy_for_windows/releases/latest";
constexpr std::size_t kMaximumResponseBytes = 4 * 1024 * 1024;

class WinHttpHandle {
public:
    explicit WinHttpHandle(HINTERNET handle = nullptr) noexcept : m_handle(handle) {}
    ~WinHttpHandle() {
        if (m_handle != nullptr) {
            WinHttpCloseHandle(m_handle);
        }
    }

    WinHttpHandle(const WinHttpHandle &) = delete;
    WinHttpHandle &operator=(const WinHttpHandle &) = delete;

    HINTERNET get() const noexcept { return m_handle; }
    explicit operator bool() const noexcept { return m_handle != nullptr; }

private:
    HINTERNET m_handle = nullptr;
};

std::wstring Utf8ToWide(std::string_view value) {
    if (value.empty()) {
        return {};
    }

    const int length = MultiByteToWideChar(
        CP_UTF8,
        MB_ERR_INVALID_CHARS,
        value.data(),
        static_cast<int>(value.size()),
        nullptr,
        0
    );
    if (length <= 0) {
        throw std::runtime_error("GitHub 返回了无效的 UTF-8 数据");
    }

    std::wstring result(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8,
            MB_ERR_INVALID_CHARS,
            value.data(),
            static_cast<int>(value.size()),
            result.data(),
            length
        ) != length) {
        throw std::runtime_error("无法解码 GitHub 返回的数据");
    }
    return result;
}

std::optional<std::string> ExtractJsonString(std::string_view json, std::string_view key) {
    const std::string marker = "\"" + std::string(key) + "\"";
    std::size_t search_from = 0;
    while (true) {
        const std::size_t key_position = json.find(marker, search_from);
        if (key_position == std::string_view::npos) {
            return std::nullopt;
        }

        std::size_t position = json.find(':', key_position + marker.size());
        if (position == std::string_view::npos) {
            return std::nullopt;
        }
        ++position;
        while (position < json.size() && std::isspace(static_cast<unsigned char>(json[position]))) {
            ++position;
        }
        if (position >= json.size() || json[position] != '"') {
            search_from = key_position + marker.size();
            continue;
        }
        ++position;

        std::string value;
        while (position < json.size()) {
            const char current = json[position++];
            if (current == '"') {
                return value;
            }
            if (current != '\\') {
                value.push_back(current);
                continue;
            }
            if (position >= json.size()) {
                return std::nullopt;
            }

            const char escaped = json[position++];
            switch (escaped) {
            case '"':
            case '\\':
            case '/':
                value.push_back(escaped);
                break;
            case 'b':
                value.push_back('\b');
                break;
            case 'f':
                value.push_back('\f');
                break;
            case 'n':
                value.push_back('\n');
                break;
            case 'r':
                value.push_back('\r');
                break;
            case 't':
                value.push_back('\t');
                break;
            case 'u':
                // The fields used by this checker are ASCII. Rejecting an
                // escaped Unicode field is safer than silently corrupting it.
                return std::nullopt;
            default:
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
}

struct SemanticVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
};

std::optional<SemanticVersion> ParseSemanticVersion(std::wstring_view value) {
    if (value.empty()) {
        return std::nullopt;
    }

    std::size_t position = 0;
    if (value[position] == L'v' || value[position] == L'V') {
        ++position;
    }

    SemanticVersion result;
    int *parts[] = {&result.major, &result.minor, &result.patch};
    for (std::size_t part = 0; part < 3; ++part) {
        if (position >= value.size() || value[position] < L'0' || value[position] > L'9') {
            return std::nullopt;
        }

        int number = 0;
        while (position < value.size() && value[position] >= L'0' && value[position] <= L'9') {
            const int digit = value[position] - L'0';
            if (number > (std::numeric_limits<int>::max() - digit) / 10) {
                return std::nullopt;
            }
            number = number * 10 + digit;
            ++position;
        }
        *parts[part] = number;

        if (part < 2) {
            if (position >= value.size() || value[position] != L'.') {
                return std::nullopt;
            }
            ++position;
        }
    }

    if (position != value.size()) {
        return std::nullopt;
    }
    return result;
}

bool IsNewer(const SemanticVersion &candidate, const SemanticVersion &current) {
    if (candidate.major != current.major) return candidate.major > current.major;
    if (candidate.minor != current.minor) return candidate.minor > current.minor;
    return candidate.patch > current.patch;
}

std::wstring WithoutLeadingV(std::wstring value) {
    if (!value.empty() && (value.front() == L'v' || value.front() == L'V')) {
        value.erase(value.begin());
    }
    return value;
}

std::string DownloadLatestRelease() {
    const std::wstring user_agent = L"maccy-for-windows/" +
        std::wstring(AppConstants::kAppVersion);
    WinHttpHandle session(WinHttpOpen(
        user_agent.c_str(),
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    ));
    if (!session) {
        throw std::runtime_error("无法创建网络会话");
    }
    if (!WinHttpSetTimeouts(session.get(), 3000, 3000, 5000, 5000)) {
        throw std::runtime_error("无法设置网络超时");
    }

    WinHttpHandle connection(WinHttpConnect(
        session.get(),
        kGitHubHost,
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    ));
    if (!connection) {
        throw std::runtime_error("无法连接 GitHub");
    }

    WinHttpHandle request(WinHttpOpenRequest(
        connection.get(),
        L"GET",
        kGitHubLatestReleasePath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    ));
    if (!request) {
        throw std::runtime_error("无法创建 GitHub 请求");
    }

    constexpr wchar_t headers[] =
        L"Accept: application/vnd.github+json\r\n"
        L"X-GitHub-Api-Version: 2022-11-28\r\n";
    if (!WinHttpAddRequestHeaders(
            request.get(),
            headers,
            static_cast<DWORD>(-1L),
            WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE
        )) {
        throw std::runtime_error("无法设置 GitHub 请求头");
    }
    if (!WinHttpSendRequest(
            request.get(),
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0
        ) || !WinHttpReceiveResponse(request.get(), nullptr)) {
        throw std::runtime_error("无法获取 GitHub Release");
    }

    DWORD status = 0;
    DWORD status_size = sizeof(status);
    if (!WinHttpQueryHeaders(
            request.get(),
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status,
            &status_size,
            WINHTTP_NO_HEADER_INDEX
        )) {
        throw std::runtime_error("无法读取 GitHub 响应状态");
    }
    if (status != 200) {
        throw std::runtime_error("GitHub 返回 HTTP " + std::to_string(status));
    }

    std::string response;
    while (true) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.get(), &available)) {
            throw std::runtime_error("无法读取 GitHub 响应");
        }
        if (available == 0) {
            break;
        }
        if (response.size() + available > kMaximumResponseBytes) {
            throw std::runtime_error("GitHub 响应过大");
        }

        const std::size_t old_size = response.size();
        response.resize(old_size + available);
        DWORD read = 0;
        if (!WinHttpReadData(
                request.get(),
                response.data() + old_size,
                available,
                &read
            )) {
            throw std::runtime_error("无法读取 GitHub 响应内容");
        }
        response.resize(old_size + read);
        if (read == 0) {
            break;
        }
    }
    return response;
}

} // namespace

bool IsNewerSemanticVersion(std::wstring_view current, std::wstring_view candidate) {
    const auto current_version = ParseSemanticVersion(current);
    const auto candidate_version = ParseSemanticVersion(candidate);
    return current_version.has_value() && candidate_version.has_value() &&
        IsNewer(*candidate_version, *current_version);
}

UpdateChecker::~UpdateChecker() {
    Stop();
}

void UpdateChecker::SetWindow(HWND window) noexcept {
    std::lock_guard lock(m_mutex);
    m_window = window;
}

bool UpdateChecker::Start(UpdateCheckMode mode) {
    {
        std::lock_guard lock(m_mutex);
        if (m_stopping || m_checking) {
            return false;
        }
    }

    // A completed worker remains joinable until the next request or shutdown.
    if (m_worker.joinable()) {
        m_worker.join();
    }

    {
        std::lock_guard lock(m_mutex);
        if (m_stopping || m_checking) {
            return false;
        }
        m_checking = true;
    }

    try {
        m_worker = std::thread(&UpdateChecker::Run, this, mode);
    } catch (...) {
        std::lock_guard lock(m_mutex);
        m_checking = false;
        return false;
    }
    return true;
}

bool UpdateChecker::IsChecking() const noexcept {
    std::lock_guard lock(m_mutex);
    return m_checking;
}

std::vector<UpdateCheckResult> UpdateChecker::TakeResults() {
    std::vector<UpdateCheckResult> results;
    std::lock_guard lock(m_mutex);
    results.swap(m_results);
    return results;
}

void UpdateChecker::Stop() noexcept {
    {
        std::lock_guard lock(m_mutex);
        m_stopping = true;
        m_window = nullptr;
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
    std::lock_guard lock(m_mutex);
    m_checking = false;
    m_results.clear();
}

void UpdateChecker::Run(UpdateCheckMode mode) {
    UpdateCheckResult result = Check(mode);
    HWND window = nullptr;
    {
        std::lock_guard lock(m_mutex);
        m_checking = false;
        if (!m_stopping) {
            m_results.push_back(std::move(result));
            window = m_window;
        }
    }
    if (window != nullptr) {
        ::PostMessageW(window, AppConstants::kUpdateCheckerResultMessage, 0, 0);
    }
}

UpdateCheckResult UpdateChecker::Check(UpdateCheckMode mode) {
    UpdateCheckResult result;
    result.mode = mode;
    result.current_version = AppConstants::kAppVersion;

    try {
        const auto current = ParseSemanticVersion(result.current_version);
        if (!current) {
            throw std::runtime_error("当前应用版本格式无效");
        }

        const std::string response = DownloadLatestRelease();
        const auto tag = ExtractJsonString(response, "tag_name");
        if (!tag || tag->empty()) {
            throw std::runtime_error("GitHub Release 缺少版本号");
        }
        result.latest_version = WithoutLeadingV(Utf8ToWide(*tag));
        const auto latest = ParseSemanticVersion(result.latest_version);
        if (!latest) {
            throw std::runtime_error("GitHub Release 版本格式无效");
        }

        if (const auto url = ExtractJsonString(response, "html_url")) {
            result.release_url = Utf8ToWide(*url);
        }
        if (result.release_url.empty()) {
            result.release_url = kGitHubReleasePage;
        }

        result.succeeded = true;
        result.update_available = IsNewerSemanticVersion(result.current_version, result.latest_version);
    } catch (const std::exception &error) {
        result.error = Utf8ToWide(error.what());
    } catch (...) {
        result.error = L"未知网络错误";
    }
    return result;
}
