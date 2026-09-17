#include "PreviewWorker.h"

#include "Constants.h"

#include <atlbase.h>

#include <stdexcept>
#include <utility>

PreviewWorker::PreviewWorker(std::filesystem::path path)
    : m_path(std::move(path)) {}

PreviewWorker::~PreviewWorker() {
    Stop();
}

void PreviewWorker::Start() {
    if (m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_readyMutex);
        m_ready = false;
        m_startupError = nullptr;
    }
    {
        std::lock_guard lock(m_requestMutex);
        m_stopping = false;
        m_latestRequest.reset();
        m_requestStopSource = std::stop_source{};
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
}

void PreviewWorker::Stop() {
    if (!m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_requestMutex);
        m_stopping = true;
        m_latestRequest.reset();
        m_requestStopSource.request_stop();
    }
    m_thread.request_stop();
    m_requestCondition.notify_all();
    m_thread.join();
    {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.clear();
    }
}

void PreviewWorker::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
}

bool PreviewWorker::Request(
    sqlite3_int64 item_id,
    std::uint64_t generation,
    UINT maximum_width,
    UINT maximum_height,
    ResultCallback callback
) {
    if (item_id <= 0 || !callback || !m_thread.joinable()) {
        return false;
    }
    {
        std::lock_guard lock(m_requestMutex);
        if (m_stopping) {
            return false;
        }
        m_requestStopSource.request_stop();
        m_requestStopSource = std::stop_source{};
        m_latestRequest = RequestData{
            item_id,
            generation,
            maximum_width,
            maximum_height,
            m_requestStopSource.get_token(),
            std::move(callback),
        };
    }
    m_requestCondition.notify_one();
    return true;
}

void PreviewWorker::DrainUiCallbacks() {
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

void PreviewWorker::ThreadMain(std::stop_token stop_token) {
    bool com_initialized = false;
    try {
        const HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
            throw std::runtime_error("Unable to initialize preview worker COM");
        }
        com_initialized = SUCCEEDED(com_result);

        Database database(m_path);
        {
            std::lock_guard lock(m_readyMutex);
            m_ready = true;
        }
        m_readyCondition.notify_one();

        while (!stop_token.stop_requested()) {
            std::optional<RequestData> request;
            {
                std::unique_lock lock(m_requestMutex);
                m_requestCondition.wait(lock, stop_token, [this] {
                    return m_stopping || m_latestRequest.has_value();
                });
                if (m_stopping || stop_token.stop_requested()) {
                    break;
                }
                request = std::move(m_latestRequest);
                m_latestRequest.reset();
            }
            if (request.has_value()) {
                ProcessRequest(database, std::move(*request), stop_token);
            }
        }

        if (com_initialized) {
            ::CoUninitialize();
        }
    } catch (...) {
        if (com_initialized) {
            ::CoUninitialize();
        }
        {
            std::lock_guard lock(m_readyMutex);
            m_startupError = std::current_exception();
            m_ready = true;
        }
        m_readyCondition.notify_one();
    }
}

void PreviewWorker::ProcessRequest(
    Database &database,
    RequestData request,
    std::stop_token worker_stop
) {
    auto result = std::make_shared<PreviewResult>();
    result->item_id = request.item_id;
    result->generation = request.generation;
    try {
        if (worker_stop.stop_requested() || request.request_stop.stop_requested()) {
            return;
        }
        result->item = database.GetItem(request.item_id, PayloadMode::Preview);
        if (result->item.has_value() &&
            !worker_stop.stop_requested() && !request.request_stop.stop_requested()) {
            bool superseded = false;
            {
                std::lock_guard lock(m_requestMutex);
                superseded = m_latestRequest.has_value() &&
                    m_latestRequest->generation != request.generation;
            }
            if (!superseded) {
                result->bitmap = DecodePreviewBitmap(
                    *result->item,
                    request.maximum_width,
                    request.maximum_height,
                    request.request_stop
                );
            }
        }
    } catch (const std::exception &error) {
        result->error = error.what();
    } catch (...) {
        result->error = "Unknown preview worker error";
    }
    if (!worker_stop.stop_requested() && !request.request_stop.stop_requested()) {
        PostResult(std::move(result), std::move(request.callback));
    }
}

void PreviewWorker::PostResult(
    std::shared_ptr<PreviewResult> result,
    ResultCallback callback
) {
    const HWND window = m_uiWindow.load(std::memory_order_acquire);
    if (window == nullptr || !callback) {
        return;
    }
    {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.push_back([result = std::move(result), callback = std::move(callback)]() mutable {
            callback(std::move(result));
        });
    }
    if (!::PostMessageW(window, AppConstants::kPreviewWorkerResultMessage, 0, 0)) {
        std::lock_guard lock(m_resultMutex);
        if (!m_uiCallbacks.empty()) {
            m_uiCallbacks.pop_back();
        }
    }
}
