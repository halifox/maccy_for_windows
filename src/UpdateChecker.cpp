#include "UpdateChecker.h"

#include "Constants.h"
#include "Localization.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <boost/system/system_error.hpp>

#include <openssl/ssl.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
namespace ssl = asio::ssl;
using tcp = asio::ip::tcp;

constexpr char kGitHubHost[] = "api.github.com";
constexpr char kGitHubPort[] = "443";
constexpr char kGitHubLatestReleaseTarget[] =
    "/repos/halifox/maccy_for_windows/releases/latest";
constexpr std::string_view kGitHubReleaseUrlPrefix =
    "https://github.com/halifox/maccy_for_windows/releases/";
constexpr wchar_t kGitHubReleasePage[] =
    L"https://github.com/halifox/maccy_for_windows/releases/latest";
constexpr std::size_t kMaximumResponseBytes = 4 * 1024 * 1024;
constexpr auto kConnectTimeout = std::chrono::seconds(3);
constexpr auto kRequestTimeout = std::chrono::seconds(5);

class LocalizedUpdateError final : public std::exception {
public:
    explicit LocalizedUpdateError(std::wstring message)
        : m_message(std::move(message)) {}

    const std::wstring &Message() const noexcept { return m_message; }

    const char *what() const noexcept override {
        return "localized update check error";
    }

private:
    std::wstring m_message;
};

struct SemanticVersion {
    int major = 0;
    int minor = 0;
    int patch = 0;
};

struct HttpResponse {
    unsigned status = 0;
    std::string body;
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

void SetTlsServerName(ssl::stream<beast::tcp_stream> &stream) {
    if (SSL_set_tlsext_host_name(stream.native_handle(), kGitHubHost) == 0) {
        throw std::runtime_error("Unable to configure the TLS server name");
    }
}

HttpResponse DownloadLatestRelease() {
    asio::io_context io_context;
    ssl::context tls_context(ssl::context::tls_client);
    tls_context.set_default_verify_paths();
    tls_context.set_verify_mode(ssl::verify_peer);

    ssl::stream<beast::tcp_stream> stream(io_context, tls_context);
    SetTlsServerName(stream);

    tcp::resolver resolver(io_context);
    const tcp::resolver::results_type endpoints = resolver.resolve(
        kGitHubHost,
        kGitHubPort
    );

    beast::get_lowest_layer(stream).expires_after(kConnectTimeout);
    beast::get_lowest_layer(stream).connect(endpoints);

    beast::get_lowest_layer(stream).expires_after(kRequestTimeout);
    stream.handshake(ssl::stream_base::client);

    http::request<http::empty_body> request(http::verb::get, kGitHubLatestReleaseTarget, 11);
    request.set(http::field::host, kGitHubHost);
    request.set(http::field::accept, "application/vnd.github+json");
    request.set(http::field::user_agent,
        "maccy-for-windows/" + std::string(AppVersion::kStringUtf8));
    request.set("X-GitHub-Api-Version", "2022-11-28");

    http::write(stream, request);

    beast::flat_buffer buffer;
    http::response_parser<http::string_body> parser;
    parser.body_limit(kMaximumResponseBytes);
    http::read(stream, buffer, parser);
    const http::response<http::string_body> response = parser.release();

    beast::error_code shutdown_error;
    stream.shutdown(shutdown_error);
    if (shutdown_error == asio::error::eof ||
        shutdown_error == ssl::error::stream_truncated) {
        shutdown_error = {};
    }
    if (shutdown_error) {
        throw boost::system::system_error(shutdown_error);
    }

    return HttpResponse{response.result_int(), response.body()};
}

std::optional<std::string> JsonString(const json::object &object, const char *key) {
    const json::value *value = object.if_contains(key);
    if (value == nullptr || !value->is_string()) {
        return std::nullopt;
    }
    const json::string &string = value->as_string();
    return std::string(string.data(), string.size());
}

json::object DownloadAndParseLatestRelease() {
    const HttpResponse response = DownloadLatestRelease();
    boost::system::error_code parse_error;
    const json::value document = json::parse(
        json::string_view(response.body.data(), response.body.size()),
        parse_error
    );

    if (response.status != 200) {
        if (!parse_error && document.is_object()) {
            const auto message = JsonString(document.as_object(), "message");
            if (message.has_value()) {
                throw LocalizedUpdateError(Localization::Format(
                    IDS_UPDATE_HTTP_ERROR,
                    {std::to_wstring(response.status), Localization::FromUtf8(*message)}
                ));
            }
        }
        throw LocalizedUpdateError(Localization::Format(
            IDS_UPDATE_HTTP_STATUS,
            {std::to_wstring(response.status)}
        ));
    }

    if (parse_error) {
        throw LocalizedUpdateError(Localization::Text(IDS_UPDATE_INVALID_JSON));
    }

    if (!document.is_object()) {
        throw LocalizedUpdateError(Localization::Text(IDS_UPDATE_INVALID_RELEASE));
    }
    return document.as_object();
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
            throw LocalizedUpdateError(Localization::Text(IDS_UPDATE_CURRENT_VERSION_INVALID));
        }

        const json::object release = DownloadAndParseLatestRelease();
        const auto tag = JsonString(release, "tag_name");
        if (!tag.has_value() || tag->empty()) {
            throw LocalizedUpdateError(Localization::Text(IDS_UPDATE_RELEASE_TAG_MISSING));
        }
        result.latest_version = WithoutLeadingV(Localization::FromUtf8(*tag));
        const auto latest = ParseSemanticVersion(result.latest_version);
        if (!latest) {
            throw LocalizedUpdateError(Localization::Text(IDS_UPDATE_RELEASE_VERSION_INVALID));
        }

        const auto url = JsonString(release, "html_url");
        if (url.has_value() && url->starts_with(kGitHubReleaseUrlPrefix)) {
            result.release_url = Localization::FromUtf8(*url);
        }
        if (result.release_url.empty()) {
            result.release_url = kGitHubReleasePage;
        }

        result.succeeded = true;
        result.update_available = IsNewerSemanticVersion(result.current_version, result.latest_version);
    } catch (const LocalizedUpdateError &error) {
        result.error = error.Message();
    } catch (const std::exception &error) {
        result.error = Localization::Format(
            IDS_UPDATE_NETWORK_ERROR,
            {Localization::FromUtf8(error.what())}
        );
    } catch (...) {
        result.error = Localization::Text(IDS_UPDATE_UNKNOWN_ERROR);
    }
    return result;
}
