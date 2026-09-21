#include "UpdateChecker.h"

#include "Constants.h"

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

constexpr char kGitHubLatestReleaseUrl[] =
    "https://api.github.com/repos/halifox/maccy_for_windows/releases/latest";
constexpr std::string_view kGitHubReleaseUrlPrefix =
    "https://github.com/halifox/maccy_for_windows/releases/";
constexpr wchar_t kGitHubReleasePage[] =
    L"https://github.com/halifox/maccy_for_windows/releases/latest";
constexpr std::size_t kMaximumResponseBytes = 4 * 1024 * 1024;

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

nlohmann::json DownloadLatestRelease() {
    std::string response;
    bool response_too_large = false;
    const auto http_response = cpr::Get(
        cpr::Url{kGitHubLatestReleaseUrl},
        cpr::Header{
            {"Accept", "application/vnd.github+json"},
            {"X-GitHub-Api-Version", "2022-11-28"},
            {"User-Agent", "maccy-for-windows/" + std::string(AppVersion::kStringUtf8)}
        },
        cpr::Timeout{5000},
        cpr::ConnectTimeout{3000},
        cpr::WriteCallback{[&response, &response_too_large](std::string_view data, std::intptr_t) {
            if (data.size() > kMaximumResponseBytes - response.size()) {
                response_too_large = true;
                return false;
            }
            response.append(data);
            return true;
        }}
    );

    if (response_too_large) {
        throw std::runtime_error("GitHub 响应过大");
    }
    if (http_response.error.code != cpr::ErrorCode::OK) {
        throw std::runtime_error("无法获取 GitHub Release：" + http_response.error.message);
    }
    if (http_response.status_code != 200) {
        const auto error = nlohmann::json::parse(response, nullptr, false);
        if (error.is_object() && error.contains("message") && error["message"].is_string()) {
            throw std::runtime_error(
                "GitHub 返回 HTTP " + std::to_string(http_response.status_code) +
                "：" + error["message"].get<std::string>()
            );
        }
        throw std::runtime_error("GitHub 返回 HTTP " + std::to_string(http_response.status_code));
    }

    auto release = nlohmann::json::parse(response, nullptr, false);
    if (release.is_discarded() || !release.is_object()) {
        throw std::runtime_error("GitHub 返回的 Release 数据格式无效");
    }
    return release;
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

        const auto release = DownloadLatestRelease();
        const auto tag = release.find("tag_name");
        if (tag == release.end() || !tag->is_string() ||
            tag->get_ref<const std::string &>().empty()) {
            throw std::runtime_error("GitHub Release 缺少版本号");
        }
        result.latest_version = WithoutLeadingV(Utf8ToWide(tag->get<std::string>()));
        const auto latest = ParseSemanticVersion(result.latest_version);
        if (!latest) {
            throw std::runtime_error("GitHub Release 版本格式无效");
        }

        const auto url = release.find("html_url");
        if (url != release.end() && url->is_string()) {
            const auto &release_url = url->get_ref<const std::string &>();
            if (release_url.starts_with(kGitHubReleaseUrlPrefix)) {
                result.release_url = Utf8ToWide(release_url);
            }
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
