#pragma once

#include "PlatformConfig.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

#include "Database.h"
#include "PreviewDecoder.h"

struct PreviewResult {
    sqlite3_int64 item_id = 0;
    std::uint64_t generation = 0;
    std::optional<ClipboardItem> item;
    std::optional<PreviewBitmap> bitmap;
    std::string error;
};

class PreviewWorker {
public:
    using ResultCallback = std::function<void(std::shared_ptr<PreviewResult>)>;

    explicit PreviewWorker(std::filesystem::path path);
    ~PreviewWorker();

    PreviewWorker(const PreviewWorker &) = delete;
    PreviewWorker &operator=(const PreviewWorker &) = delete;

    void Start();
    void Stop();
    void SetUiWindow(HWND window) noexcept;
    bool Request(
        sqlite3_int64 item_id,
        std::uint64_t generation,
        UINT maximum_width,
        UINT maximum_height,
        ResultCallback callback
    );
    void DrainUiCallbacks();

private:
    struct RequestData {
        sqlite3_int64 item_id = 0;
        std::uint64_t generation = 0;
        UINT maximum_width = 0;
        UINT maximum_height = 0;
        std::stop_token request_stop;
        ResultCallback callback;
    };

    void ThreadMain(std::stop_token stop_token);
    void ProcessRequest(Database &database, RequestData request,
                        std::stop_token worker_stop);
    void PostResult(std::shared_ptr<PreviewResult> result, ResultCallback callback);

    std::filesystem::path m_path;
    std::jthread m_thread;
    std::atomic<HWND> m_uiWindow{nullptr};

    std::mutex m_requestMutex;
    std::condition_variable_any m_requestCondition;
    std::optional<RequestData> m_latestRequest;
    std::stop_source m_requestStopSource;
    bool m_stopping = false;

    std::mutex m_resultMutex;
    std::deque<std::function<void()>> m_uiCallbacks;

    std::mutex m_readyMutex;
    std::condition_variable m_readyCondition;
    bool m_ready = false;
    std::exception_ptr m_startupError;
};
