#pragma once

#include "PlatformConfig.h"

#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "Database.h"
#include "PreviewWindow.h"

struct PreviewResult {
    sqlite3_int64 item_id = 0;
    std::uint64_t generation = 0;
    bool displayed = false;
    std::string error;
};

class PreviewWorker {
public:
    using ResultCallback = std::function<void(std::shared_ptr<PreviewResult>)>;

    explicit PreviewWorker(std::filesystem::path path);
    ~PreviewWorker();

    PreviewWorker(const PreviewWorker &) = delete;
    PreviewWorker &operator=(const PreviewWorker &) = delete;

    void Start(HWND owner);
    void Stop();
    void SetUiWindow(HWND window) noexcept;
    void Hide();
    void Reposition();
    bool IsVisible() const noexcept;
    bool ContainsWindow(HWND window) const noexcept;
    bool Request(
        sqlite3_int64 item_id,
        std::uint64_t generation,
        ResultCallback callback
    );
    void DrainUiCallbacks();

private:
    struct RequestData {
        sqlite3_int64 item_id = 0;
        std::uint64_t generation = 0;
        std::stop_token request_stop;
        ResultCallback callback;
    };

    void ThreadMain(std::stop_token stop_token);
    void ProcessRequest(Database &database, PreviewWindow &preview, RequestData request,
                        std::stop_token worker_stop);
    void PostResult(std::shared_ptr<PreviewResult> result, ResultCallback callback);
    void FailPendingRequest(std::string error);
    bool PostCommand(WPARAM command) const noexcept;

    std::filesystem::path m_path;
    std::jthread m_thread;
    std::atomic<DWORD> m_threadId{0};
    std::atomic<bool> m_workerReady{false};
    std::atomic<bool> m_workerFailed{false};
    std::atomic<HWND> m_ownerWindow{nullptr};
    std::atomic<HWND> m_uiWindow{nullptr};
    std::atomic<HWND> m_previewWindow{nullptr};
    std::atomic<bool> m_previewVisible{false};

    std::mutex m_requestMutex;
    std::optional<RequestData> m_latestRequest;
    std::stop_source m_requestStopSource;
    bool m_stopping = false;

    std::mutex m_resultMutex;
    std::deque<std::function<void()>> m_uiCallbacks;
};
