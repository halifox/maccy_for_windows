#include "ThumbnailWorker.h"

#include "Constants.h"

#include <atlbase.h>

#include <memory>
#include <utility>

ThumbnailWorker::~ThumbnailWorker() {
    Stop();
}

void ThumbnailWorker::Start(HWND window) {
    if (window == nullptr || m_thread.joinable()) {
        return;
    }
    m_stopping = false;
    m_uiWindow.store(window, std::memory_order_release);
    m_thread = std::jthread([this](std::stop_token stop_token) {
        ThreadMain(stop_token);
    });
}

void ThumbnailWorker::Stop() noexcept {
    if (!m_thread.joinable()) {
        m_uiWindow.store(nullptr, std::memory_order_release);
        return;
    }
    {
        std::lock_guard lock(m_jobMutex);
        m_stopping = true;
        m_jobs.clear();
    }
    m_thread.request_stop();
    m_jobCondition.notify_all();
    m_thread.join();
    m_uiWindow.store(nullptr, std::memory_order_release);
    std::lock_guard lock(m_resultMutex);
    m_uiCallbacks.clear();
}

void ThumbnailWorker::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
}

void ThumbnailWorker::Submit(Request request, ResultCallback callback) {
    if (request.item_id <= 0 || !callback || !m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_jobMutex);
        if (m_stopping) {
            return;
        }
        m_jobs.push_back(Job{std::move(request), std::move(callback)});
    }
    m_jobCondition.notify_one();
}

void ThumbnailWorker::DrainUiCallbacks() {
    std::deque<std::function<void()>> callbacks;
    {
        std::lock_guard lock(m_resultMutex);
        callbacks.swap(m_uiCallbacks);
    }
    for (auto &callback : callbacks) {
        if (callback) {
            callback();
        }
    }
}

void ThumbnailWorker::ThreadMain(std::stop_token stop_token) {
    const HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
        return;
    }
    const bool com_initialized = SUCCEEDED(com_result);
    const bool com_available = com_initialized || com_result == RPC_E_CHANGED_MODE;
    while (!stop_token.stop_requested()) {
        Job job;
        {
            std::unique_lock lock(m_jobMutex);
            m_jobCondition.wait(lock, [&] {
                return stop_token.stop_requested() || !m_jobs.empty();
            });
            if (stop_token.stop_requested()) {
                break;
            }
            job = std::move(m_jobs.front());
            m_jobs.pop_front();
        }
        if (!com_available || stop_token.stop_requested()) {
            continue;
        }
        auto bitmap = DecodePreviewBitmap(
            job.request.item,
            job.request.maximum_width,
            job.request.maximum_height,
            stop_token
        );
        if (stop_token.stop_requested()) {
            break;
        }
        PostResult(
            job.request.item_id,
            job.request.generation,
            std::move(bitmap),
            std::move(job.callback)
        );
    }
    if (com_initialized) {
        ::CoUninitialize();
    }
}

void ThumbnailWorker::PostResult(
    sqlite3_int64 item_id,
    std::uint64_t generation,
    std::optional<PreviewBitmap> bitmap,
    ResultCallback callback
) {
    const HWND window = m_uiWindow.load(std::memory_order_acquire);
    if (window == nullptr || !callback) {
        return;
    }
    {
        std::lock_guard lock(m_resultMutex);
        auto result = std::make_shared<std::optional<PreviewBitmap>>(std::move(bitmap));
        m_uiCallbacks.push_back([
            item_id,
            generation,
            result = std::move(result),
            callback = std::move(callback)
        ]() mutable {
            callback(item_id, generation, std::move(*result));
        });
    }
    ::PostMessageW(window, AppConstants::kThumbnailWorkerResultMessage, 0, 0);
}
