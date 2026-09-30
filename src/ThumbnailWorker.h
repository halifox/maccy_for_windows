#pragma once

#include "PlatformConfig.h"
#include "PreviewDecoder.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

class ThumbnailWorker {
public:
    struct Request {
        sqlite3_int64 item_id = 0;
        std::uint64_t generation = 0;
        ClipboardItem item;
        UINT maximum_width = 0;
        UINT maximum_height = 0;
    };

    using ResultCallback = std::function<void(
        sqlite3_int64,
        std::uint64_t,
        std::optional<PreviewBitmap>
    )>;

    ThumbnailWorker() = default;
    ~ThumbnailWorker();

    ThumbnailWorker(const ThumbnailWorker &) = delete;
    ThumbnailWorker &operator=(const ThumbnailWorker &) = delete;

    void Start(HWND window);
    void Stop() noexcept;
    void SetUiWindow(HWND window) noexcept;
    void Submit(Request request, ResultCallback callback);
    void DrainUiCallbacks();

private:
    struct Job {
        Request request;
        ResultCallback callback;
    };

    void ThreadMain(std::stop_token stop_token);
    void PostResult(sqlite3_int64 item_id, std::uint64_t generation,
                    std::optional<PreviewBitmap> bitmap, ResultCallback callback);

    std::jthread m_thread;
    std::mutex m_jobMutex;
    std::condition_variable m_jobCondition;
    std::deque<Job> m_jobs;
    bool m_stopping = false;

    std::atomic<HWND> m_uiWindow{nullptr};
    std::mutex m_resultMutex;
    std::deque<std::function<void()>> m_uiCallbacks;
};
