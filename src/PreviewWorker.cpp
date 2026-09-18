#include "PreviewWorker.h"

#include "Constants.h"
#include "ClipboardRules.h"
#include "PreviewDecoder.h"

#include <atlbase.h>

#include <algorithm>
#include <climits>
#include <stdexcept>
#include <utility>

namespace {

constexpr WPARAM kRequestCommand = 1;
constexpr WPARAM kHideCommand = 2;
constexpr WPARAM kRepositionCommand = 3;

std::wstring FullText(const ClipboardItem &item) {
    for (const ClipboardFormatData &data : item.data) {
        if (!ClipboardRules::IsUnicodeTextFormat(data)) {
            continue;
        }
        std::wstring text = ClipboardRules::DecodeUnicodeText(data.bytes);
        text.resize(std::min(text.size(), ClipboardRules::Limits::kMaximumPreviewTextCharacters));
        return text;
    }

    for (const ClipboardFormatData &data : item.data) {
        if (!ClipboardRules::IsAnsiTextFormat(data)) {
            continue;
        }
        std::wstring text = ClipboardRules::DecodeAnsiText(data.bytes);
        text.resize(std::min(text.size(), ClipboardRules::Limits::kMaximumPreviewTextCharacters));
        return text;
    }

    return item.preview;
}

void PositionPreviewWindow(HWND owner, PreviewWindow &preview, bool show) {
    const HWND preview_handle = preview.Window();
    if (owner == nullptr || preview_handle == nullptr || !::IsWindow(preview_handle)) {
        return;
    }

    RECT main_rect{};
    RECT preview_rect{};
    if (!::GetWindowRect(owner, &main_rect) ||
        !::GetWindowRect(preview_handle, &preview_rect)) {
        return;
    }

    const int width = preview_rect.right - preview_rect.left;
    const int height = preview_rect.bottom - preview_rect.top;
    if (width <= 0 || height <= 0) {
        return;
    }
    HMONITOR monitor = ::MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitor_info{sizeof(monitor_info)};
    if (monitor == nullptr || !::GetMonitorInfoW(monitor, &monitor_info)) {
        return;
    }

    const RECT &work_area = monitor_info.rcWork;
    constexpr int gap = 0;
    int x = main_rect.right + gap;
    int y = main_rect.top;
    const bool fits_right = main_rect.right + gap + width <= work_area.right;
    const bool fits_left = main_rect.left - gap - width >= work_area.left;
    if (fits_right) {
        x = main_rect.right + gap;
    } else if (fits_left) {
        x = main_rect.left - width - gap;
    } else {
        const bool fits_above = main_rect.top - gap - height >= work_area.top;
        const bool fits_below = main_rect.bottom + gap + height <= work_area.bottom;
        x = std::clamp(
            static_cast<int>(main_rect.left),
            static_cast<int>(work_area.left),
            std::max<int>(work_area.left, work_area.right - width)
        );
        if (fits_above) {
            y = main_rect.top - height - gap;
        } else if (fits_below) {
            y = main_rect.bottom + gap;
        }
    }
    const int max_x = std::max<int>(work_area.left, work_area.right - width);
    const int max_y = std::max<int>(work_area.top, work_area.bottom - height);
    x = std::clamp(x, static_cast<int>(work_area.left), max_x);
    y = std::clamp(y, static_cast<int>(work_area.top), max_y);

    const UINT flags = SWP_NOSIZE | SWP_NOACTIVATE | (show ? SWP_SHOWWINDOW : 0);
    ::SetWindowPos(preview_handle, HWND_TOPMOST, x, y, 0, 0, flags);
}

} // namespace

PreviewWorker::PreviewWorker(std::filesystem::path path)
    : m_path(std::move(path)) {}

PreviewWorker::~PreviewWorker() {
    Stop();
}

void PreviewWorker::Start(HWND owner) {
    if (owner == nullptr) {
        throw std::invalid_argument("Preview worker owner window is null");
    }
    if (m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_requestMutex);
        m_stopping = false;
        m_latestRequest.reset();
        m_requestStopSource = std::stop_source{};
    }
    m_threadId.store(0, std::memory_order_release);
    m_workerReady.store(false, std::memory_order_release);
    m_workerFailed.store(false, std::memory_order_release);
    m_previewWindow.store(nullptr, std::memory_order_release);
    m_previewVisible.store(false, std::memory_order_release);
    m_ownerWindow.store(owner, std::memory_order_release);
    m_uiWindow.store(owner, std::memory_order_release);

    m_thread = std::jthread([this](std::stop_token stop_token) {
        ThreadMain(stop_token);
    });
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
    const DWORD thread_id = m_threadId.load(std::memory_order_acquire);
    if (thread_id != 0) {
        ::PostThreadMessageW(thread_id, WM_QUIT, 0, 0);
    }
    m_thread.join();
    m_threadId.store(0, std::memory_order_release);
    m_workerReady.store(false, std::memory_order_release);
    m_workerFailed.store(false, std::memory_order_release);
    m_previewWindow.store(nullptr, std::memory_order_release);
    m_previewVisible.store(false, std::memory_order_release);
    {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.clear();
    }
    m_ownerWindow.store(nullptr, std::memory_order_release);
    m_uiWindow.store(nullptr, std::memory_order_release);
}

void PreviewWorker::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
}

void PreviewWorker::Hide() {
    if (!m_thread.joinable()) {
        return;
    }
    {
        std::lock_guard lock(m_requestMutex);
        m_requestStopSource.request_stop();
        m_latestRequest.reset();
    }
    PostCommand(kHideCommand);
}

void PreviewWorker::Reposition() {
    if (m_thread.joinable()) {
        PostCommand(kRepositionCommand);
    }
}

bool PreviewWorker::IsVisible() const noexcept {
    return m_previewVisible.load(std::memory_order_acquire);
}

bool PreviewWorker::ContainsWindow(HWND window) const noexcept {
    const HWND preview = m_previewWindow.load(std::memory_order_acquire);
    if (window == nullptr || preview == nullptr) {
        return false;
    }
    return window == preview || ::GetAncestor(window, GA_ROOT) == preview;
}

bool PreviewWorker::Request(
    sqlite3_int64 item_id,
    std::uint64_t generation,
    ResultCallback callback
) {
    if (item_id <= 0 || !callback || !m_thread.joinable() ||
        m_workerFailed.load(std::memory_order_acquire)) {
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
            m_requestStopSource.get_token(),
            std::move(callback),
        };
    }
    if (PostCommand(kRequestCommand) || !m_workerReady.load(std::memory_order_acquire)) {
        return true;
    }
    {
        std::lock_guard lock(m_requestMutex);
        if (m_latestRequest.has_value() && m_latestRequest->generation == generation) {
            m_latestRequest.reset();
        }
    }
    return false;
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

bool PreviewWorker::PostCommand(WPARAM command) const noexcept {
    const DWORD thread_id = m_threadId.load(std::memory_order_acquire);
    return thread_id != 0 && ::PostThreadMessageW(
        thread_id,
        AppConstants::kPreviewWorkerCommandMessage,
        command,
        0
    ) != FALSE;
}

void PreviewWorker::ThreadMain(std::stop_token stop_token) {
    bool com_initialized = false;
    PreviewWindow preview;
    std::string worker_error;
    try {
        const HRESULT com_result = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(com_result) && com_result != RPC_E_CHANGED_MODE) {
            throw std::runtime_error("Unable to initialize preview worker COM");
        }
        com_initialized = SUCCEEDED(com_result);

        Database database(m_path);
        const HWND owner = m_ownerWindow.load(std::memory_order_acquire);
        if (owner == nullptr || !preview.Initialize(owner)) {
            throw std::runtime_error("Unable to create preview window");
        }
        m_previewWindow.store(preview.Window(), std::memory_order_release);

        MSG message{};
        ::PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        m_threadId.store(::GetCurrentThreadId(), std::memory_order_release);
        m_workerReady.store(true, std::memory_order_release);

        bool request_pending = false;
        {
            std::lock_guard lock(m_requestMutex);
            request_pending = !m_stopping && m_latestRequest.has_value();
        }
        if (request_pending) {
            PostCommand(kRequestCommand);
        }

        while (!stop_token.stop_requested()) {
            const BOOL result = ::GetMessageW(&message, nullptr, 0, 0);
            if (result == 0) {
                break;
            }
            if (result < 0) {
                worker_error = "Preview worker message loop failed";
                break;
            }
            if (message.message == AppConstants::kPreviewWorkerCommandMessage) {
                switch (message.wParam) {
                case kRequestCommand: {
                    std::optional<RequestData> request;
                    {
                        std::lock_guard lock(m_requestMutex);
                        request = std::move(m_latestRequest);
                        m_latestRequest.reset();
                    }
                    if (request.has_value()) {
                        ProcessRequest(database, preview, std::move(*request), stop_token);
                    }
                    break;
                }
                case kHideCommand:
                    preview.Hide();
                    m_previewVisible.store(false, std::memory_order_release);
                    break;
                case kRepositionCommand:
                    if (preview.IsVisible()) {
                        PositionPreviewWindow(owner, preview, false);
                    }
                    break;
                default:
                    break;
                }
                continue;
            }
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
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

    if (preview.Window() != nullptr && ::IsWindow(preview.Window())) {
        preview.DestroyWindow();
    }
    m_workerReady.store(false, std::memory_order_release);
    m_previewVisible.store(false, std::memory_order_release);
    m_previewWindow.store(nullptr, std::memory_order_release);
    m_threadId.store(0, std::memory_order_release);
    if (com_initialized) {
        ::CoUninitialize();
    }
}

void PreviewWorker::ProcessRequest(
    Database &database,
    PreviewWindow &preview,
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

        const auto item = database.GetItem(request.item_id, PayloadMode::Preview);
        if (!item.has_value()) {
            if (worker_stop.stop_requested() || request.request_stop.stop_requested()) {
                return;
            }
            preview.Hide();
            m_previewVisible.store(false, std::memory_order_release);
            PostResult(std::move(result), std::move(request.callback));
            return;
        }

        std::wstring text = FullText(*item);
        if (worker_stop.stop_requested() || request.request_stop.stop_requested()) {
            return;
        }

        UINT maximum_width = 0;
        UINT maximum_height = 0;
        preview.GetImageSize(maximum_width, maximum_height);
        auto bitmap = DecodePreviewBitmap(
            *item,
            maximum_width,
            maximum_height,
            request.request_stop
        );
        if (worker_stop.stop_requested() || request.request_stop.stop_requested()) {
            return;
        }

        PreviewBitmap preview_bitmap;
        if (bitmap.has_value()) {
            preview_bitmap = std::move(*bitmap);
        }
        preview.SetItem(*item, std::move(text), std::move(preview_bitmap));
        PositionPreviewWindow(m_ownerWindow.load(std::memory_order_acquire), preview, true);
        m_previewVisible.store(true, std::memory_order_release);
        result->displayed = true;
    } catch (const std::exception &error) {
        result->error = error.what();
    } catch (...) {
        result->error = "Unknown preview worker error";
    }

    if (worker_stop.stop_requested() || request.request_stop.stop_requested()) {
        return;
    }
    if (!result->displayed) {
        preview.Hide();
        m_previewVisible.store(false, std::memory_order_release);
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
