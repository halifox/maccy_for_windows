#include "PreviewWorker.h"

#include "Constants.h"
#include "ClipboardRules.h"
#include "Database.h"
#include "PreviewDecoder.h"

#include <windows.h>

#include <stdexcept>
#include <utility>

namespace {

std::wstring FullText(const ClipboardItem &item) {
    if (item.has_files) {
        std::wstring paths;
        for (const auto &path : item.file_paths) {
            if (!paths.empty()) paths += L"\r\n";
            paths += path;
        }
        return paths.empty() ? item.preview : paths;
    }
    for (const ClipboardFormatData &data : item.data) {
        if (!ClipboardRules::IsUnicodeTextFormat(data)) {
            continue;
        }
        return ClipboardRules::DecodeUnicodeText(
            data.bytes,
            ClipboardRules::Limits::kMaximumPreviewTextCharacters
        );
    }

    for (const ClipboardFormatData &data : item.data) {
        if (!ClipboardRules::IsAnsiTextFormat(data)) {
            continue;
        }
        return ClipboardRules::DecodeAnsiText(
            data.bytes,
            ClipboardRules::Limits::kMaximumPreviewTextCharacters
        );
    }

    return item.preview;
}

} // namespace

PreviewWorker::PreviewWorker(std::filesystem::path path)
    : m_path(std::move(path)) {}

PreviewWorker::~PreviewWorker() {
    Stop();
}

void PreviewWorker::Start(HWND ui_window) {
    if (ui_window == nullptr) {
        throw std::invalid_argument("Preview worker UI window is null");
    }
    if (m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_requestMutex);
        m_stopping = false;
        m_latestRequest.reset();
        m_requestStopSource = std::stop_source{};
        m_started = true;
    }
    m_workerFailed.store(false, std::memory_order_release);
    m_uiWindow.store(ui_window, std::memory_order_release);

    m_thread = std::jthread([this](std::stop_token stop_token) {
        ThreadMain(stop_token);
    });
}

void PreviewWorker::Stop() {
    if (!m_thread.joinable()) {
        m_uiWindow.store(nullptr, std::memory_order_release);
        return;
    }
    {
        std::lock_guard lock(m_requestMutex);
        m_stopping = true;
        m_latestRequest.reset();
        m_requestStopSource.request_stop();
        m_started = false;
    }
    m_thread.request_stop();
    m_requestCondition.notify_all();
    m_thread.join();
    m_workerFailed.store(false, std::memory_order_release);
    {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.clear();
    }
    m_uiWindow.store(nullptr, std::memory_order_release);
}

void PreviewWorker::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
    if (window == nullptr) {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.clear();
    }
}

bool PreviewWorker::Request(
    sqlite3_int64 item_id,
    std::uint64_t generation,
    UINT maximum_width,
    UINT maximum_height,
    ResultCallback callback
) {
    if (item_id <= 0 || !callback || !m_thread.joinable() ||
        m_workerFailed.load(std::memory_order_acquire)) {
        return false;
    }
    std::optional<RequestData> superseded;
    {
        std::lock_guard lock(m_requestMutex);
        if (m_stopping || !m_started) {
            return false;
        }
        m_requestStopSource.request_stop();
        m_requestStopSource = std::stop_source{};
        if (m_latestRequest.has_value()) {
            superseded = std::move(*m_latestRequest);
            m_latestRequest.reset();
        }
        m_latestRequest = RequestData{
            item_id,
            generation,
            maximum_width,
            maximum_height,
            m_requestStopSource.get_token(),
            std::move(callback),
        };
    }
    if (superseded.has_value()) {
        auto cancelled = std::make_shared<PreviewResult>();
        cancelled->item_id = superseded->item_id;
        cancelled->generation = superseded->generation;
        PostResult(std::move(cancelled), std::move(superseded->callback));
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
    std::string worker_error;
    try {
        const HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
            throw std::runtime_error("Unable to initialize preview worker COM");
        }
        com_initialized = SUCCEEDED(com_result);

        Database database(m_path);

        while (!stop_token.stop_requested()) {
            RequestData request;
            {
                std::unique_lock lock(m_requestMutex);
                m_requestCondition.wait(lock, [&] {
                    return stop_token.stop_requested() || m_latestRequest.has_value();
                });
                if (stop_token.stop_requested()) {
                    break;
                }
                request = std::move(*m_latestRequest);
                m_latestRequest.reset();
            }
            ProcessRequest(database, std::move(request), stop_token);
        }
    } catch (const std::exception &error) {
        worker_error = error.what();
    } catch (...) {
        worker_error = "Unknown preview worker error";
    }

    if (!worker_error.empty()) {
        m_workerFailed.store(true, std::memory_order_release);
        OutputDebugStringA(("Preview worker stopped: " + worker_error + "\n").c_str());
        FailPendingRequest(worker_error);
    }

    if (com_initialized) {
        ::CoUninitialize();
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
    const auto cancelled = [&]() {
        return worker_stop.stop_requested() || request.request_stop.stop_requested();
    };
    try {
        if (cancelled()) {
            PostResult(std::move(result), std::move(request.callback));
            return;
        }

        std::optional<ClipboardItem> item;
        // Retry briefly when the storage writer holds the DB (M-05).
        for (int attempt = 0; attempt < 5; ++attempt) {
            if (cancelled()) {
                PostResult(std::move(result), std::move(request.callback));
                return;
            }
            try {
                item = database.GetItem(request.item_id, PayloadMode::Preview);
                break;
            } catch (const std::exception &) {
                if (attempt + 1 >= 5) {
                    throw;
                }
                ::Sleep(25 * (attempt + 1));
            }
        }
        if (!item.has_value()) {
            PostResult(std::move(result), std::move(request.callback));
            return;
        }

        result->text = FullText(*item);
        if (cancelled()) {
            PostResult(std::move(result), std::move(request.callback));
            return;
        }

        const UINT maximum_width = request.maximum_width == 0 ? 194 : request.maximum_width;
        const UINT maximum_height = request.maximum_height == 0 ? 98 : request.maximum_height;
        auto bitmap = DecodePreviewBitmap(
            *item,
            maximum_width,
            maximum_height,
            request.request_stop
        );
        if (cancelled()) {
            PostResult(std::move(result), std::move(request.callback));
            return;
        }

        if (bitmap.has_value()) {
            result->bitmap = std::move(*bitmap);
        }
        result->item = std::move(*item);
        result->displayed = true;
    } catch (const std::exception &error) {
        result->error = error.what();
    } catch (...) {
        result->error = "Unknown preview worker error";
    }

    PostResult(std::move(result), std::move(request.callback));
}

void PreviewWorker::FailPendingRequest(std::string error) {
    std::optional<RequestData> request;
    {
        std::lock_guard lock(m_requestMutex);
        request = std::move(m_latestRequest);
        m_latestRequest.reset();
    }
    if (!request.has_value()) {
        return;
    }

    auto result = std::make_shared<PreviewResult>();
    result->item_id = request->item_id;
    result->generation = request->generation;
    result->error = std::move(error);
    PostResult(std::move(result), std::move(request->callback));
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
    ::PostMessageW(window, AppConstants::kPreviewWorkerResultMessage, 0, 0);
}
