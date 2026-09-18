#pragma once

#include "PlatformConfig.h"

#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

enum class UpdateCheckMode {
    Manual,
    Automatic,
};

struct UpdateCheckResult {
    UpdateCheckMode mode = UpdateCheckMode::Manual;
    bool succeeded = false;
    bool update_available = false;
    std::wstring current_version;
    std::wstring latest_version;
    std::wstring release_url;
    std::wstring error;
};

bool IsNewerSemanticVersion(std::wstring_view current, std::wstring_view candidate);

class UpdateChecker {
public:
    UpdateChecker() = default;
    ~UpdateChecker();

    UpdateChecker(const UpdateChecker &) = delete;
    UpdateChecker &operator=(const UpdateChecker &) = delete;

    void SetWindow(HWND window) noexcept;
    bool Start(UpdateCheckMode mode);
    bool IsChecking() const noexcept;
    std::vector<UpdateCheckResult> TakeResults();
    void Stop() noexcept;

private:
    void Run(UpdateCheckMode mode);
    static UpdateCheckResult Check(UpdateCheckMode mode);

    mutable std::mutex m_mutex;
    HWND m_window = nullptr;
    bool m_stopping = false;
    bool m_checking = false;
    std::thread m_worker;
    std::vector<UpdateCheckResult> m_results;
};
