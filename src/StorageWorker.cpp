#include "StorageWorker.h"

#include "ClipboardMonitor.h"
#include "Constants.h"

#include <windows.h>

#include <stdexcept>
#include <utility>

namespace {

constexpr wchar_t kStorageWorkerWindowClass[] = L"maccy.StorageWorkerWindow";
constexpr size_t kMaximumQueuedTasks = 256;

void RegisterWorkerWindowClass() {
    WNDCLASSEXW window_class{sizeof(window_class)};
    window_class.lpfnWndProc = &StorageWorker::WindowProc;
    window_class.hInstance = ::GetModuleHandleW(nullptr);
    window_class.lpszClassName = kStorageWorkerWindowClass;
    if (::RegisterClassExW(&window_class) == 0 && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error("Unable to register storage worker window class");
    }
}

} // namespace

StorageWorker::StorageWorker(std::filesystem::path path, bool monitor_clipboard)
    : m_path(std::move(path)), m_monitorClipboard(monitor_clipboard) {}

StorageWorker::~StorageWorker() {
    Stop();
}

StorageInitialState StorageWorker::Start() {
    if (m_thread.joinable()) {
        std::lock_guard lock(m_readyMutex);
        return m_initialState;
    }

    {
        std::lock_guard lock(m_readyMutex);
        m_ready = false;
        m_startupError = nullptr;
        m_initialState = {};
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
    m_accepting.store(true, std::memory_order_release);
    return m_initialState;
}

void StorageWorker::Stop() {
    if (!m_thread.joinable()) {
        return;
    }

    m_accepting.store(false, std::memory_order_release);
    m_thread.request_stop();
    if (const HWND window = m_workerWindow.load(std::memory_order_acquire);
        window != nullptr) {
        ::PostMessageW(window, AppConstants::kStorageWorkerShutdownMessage, 0, 0);
    }
    m_thread.join();
    m_workerWindow.store(nullptr, std::memory_order_release);

    {
        std::lock_guard lock(m_commandMutex);
        m_commands.clear();
        m_latestSearch.reset();
    }
    {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.clear();
    }
}

void StorageWorker::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
}

bool StorageWorker::Enqueue(QueuedTask task, bool latest_search) {
    if (!task.task || !m_accepting.load(std::memory_order_acquire)) {
        return false;
    }

    std::lock_guard lock(m_commandMutex);
    if (!m_accepting.load(std::memory_order_relaxed)) {
        return false;
    }

    const HWND window = m_workerWindow.load(std::memory_order_acquire);
    if (window == nullptr) {
        return false;
    }

    if (latest_search) {
        std::optional<QueuedTask> previous = std::move(m_latestSearch);
        m_latestSearch = std::move(task);
        if (::PostMessageW(window, AppConstants::kStorageWorkerCommandMessage, 0, 0) != FALSE) {
            return true;
        }
        m_latestSearch = std::move(previous);
        return false;
    }

    if (m_commands.size() >= kMaximumQueuedTasks) {
        return false;
    }
    m_commands.push_back(std::move(task));
    if (::PostMessageW(window, AppConstants::kStorageWorkerCommandMessage, 0, 0) != FALSE) {
        return true;
    }
    m_commands.pop_back();
    return false;
}

bool StorageWorker::Post(Task task, ErrorHandler on_error, SuccessHandler on_success) {
    return Enqueue(
        QueuedTask{std::move(task), std::move(on_error), std::move(on_success)},
        false
    );
}

bool StorageWorker::PostLatestSearch(
    Task task,
    ErrorHandler on_error,
    SuccessHandler on_success
) {
    return Enqueue(
        QueuedTask{std::move(task), std::move(on_error), std::move(on_success)},
        true
    );
}

bool StorageWorker::PostToUi(UiCallback callback) {
    if (!callback) {
        return false;
    }
    const HWND window = m_uiWindow.load(std::memory_order_acquire);
    if (window == nullptr) {
        return false;
    }

    std::lock_guard lock(m_resultMutex);
    m_uiCallbacks.push_back(std::move(callback));
    if (::PostMessageW(window, AppConstants::kStorageWorkerResultMessage, 0, 0)) {
        return true;
    }
    m_uiCallbacks.pop_back();
    return false;
}

void StorageWorker::SetErrorHandler(ErrorHandler callback) {
    std::lock_guard lock(m_handlerMutex);
    m_errorHandler = std::move(callback);
}

void StorageWorker::ReportError(ErrorHandler on_error, std::string message) {
    if (!on_error) {
        std::lock_guard lock(m_handlerMutex);
        on_error = m_errorHandler;
    }
    if (!on_error) {
        ::OutputDebugStringA(message.c_str());
        ::OutputDebugStringA("\n");
        return;
    }

    const std::string error_message = message;
    if (!PostToUi([on_error = std::move(on_error), message = error_message]() {
        on_error(message);
    })) {
        ::OutputDebugStringA(error_message.c_str());
        ::OutputDebugStringA("\n");
    }
}

void StorageWorker::DrainUiCallbacks() {
    std::deque<UiCallback> callbacks;
    {
        std::lock_guard lock(m_resultMutex);
        callbacks.swap(m_uiCallbacks);
    }
    for (UiCallback &callback : callbacks) {
        if (callback) {
            callback();
        }
    }
}

void StorageWorker::SetHistoryChangedHandler(UiCallback callback) {
    std::lock_guard lock(m_handlerMutex);
    m_historyChangedHandler = std::move(callback);
}

void StorageWorker::SetSettingsChangedHandler(SettingsChangedCallback callback) {
    std::lock_guard lock(m_handlerMutex);
    m_settingsChangedHandler = std::move(callback);
}

LRESULT CALLBACK StorageWorker::WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    auto *worker = reinterpret_cast<StorageWorker *>(::GetWindowLongPtrW(
        window,
        GWLP_USERDATA
    ));
    if (message == WM_NCCREATE) {
        const auto *create = reinterpret_cast<const CREATESTRUCTW *>(lParam);
        worker = static_cast<StorageWorker *>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(worker));
    }
    if (worker == nullptr) {
        return ::DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message) {
    case AppConstants::kStorageWorkerCommandMessage:
        worker->ProcessTasks();
        return 0;
    case AppConstants::kStorageWorkerShutdownMessage:
        worker->HandleShutdown();
        return 0;
    case WM_CLIPBOARDUPDATE:
        worker->HandleClipboardUpdate();
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

void StorageWorker::ThreadMain(std::stop_token stop_token) {
    const std::stop_callback wake_on_stop(stop_token, [this] {
        if (const HWND window = m_workerWindow.load(std::memory_order_acquire);
            window != nullptr) {
            ::PostMessageW(window, AppConstants::kStorageWorkerShutdownMessage, 0, 0);
        }
    });
    try {
        RegisterWorkerWindowClass();
        const HWND window = ::CreateWindowExW(
            0,
            kStorageWorkerWindowClass,
            L"maccy storage worker",
            0,
            0,
            0,
            0,
            0,
            HWND_MESSAGE,
            nullptr,
            ::GetModuleHandleW(nullptr),
            this
        );
        if (window == nullptr) {
            throw std::runtime_error("Unable to create storage worker window");
        }
        m_workerWindow.store(window, std::memory_order_release);

        Database database(m_path);
        AppSettings settings = AppSettings::Load(database);
        ClipboardMonitor clipboard(database, settings);
        if (m_monitorClipboard && !clipboard.Initialize(window)) {
            throw std::runtime_error("Unable to register clipboard listener");
        }
        clipboard.ReloadIgnoreLists();

        StorageInitialState initial;
        initial.settings = settings;
        initial.suppress_clear_alert =
            database.GetSetting(L"behavior.suppressClearAlert") == std::optional<std::wstring>(L"1");
        initial.ignored_lists[0] = database.GetList(DatabaseList::IgnoredApplications);
        initial.ignored_lists[1] = database.GetList(DatabaseList::IgnoredFormats);
        initial.ignored_lists[2] = database.GetList(DatabaseList::IgnoredRegexps);

        StorageContext context{database, clipboard, settings};
        m_context = &context;
        SignalReady(std::move(initial));

        MSG message{};
        for (;;) {
            const BOOL result = ::GetMessageW(&message, nullptr, 0, 0);
            if (result <= 0) {
                break;
            }
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        clipboard.Shutdown();
        m_context = nullptr;
        ::DestroyWindow(window);
        m_workerWindow.store(nullptr, std::memory_order_release);
    } catch (...) {
        SignalStartupFailure(std::current_exception());
    }
}

void StorageWorker::ProcessTasks(bool include_latest_search) {
    if (m_context == nullptr) {
        return;
    }

    std::deque<QueuedTask> tasks;
    {
        std::lock_guard lock(m_commandMutex);
        tasks.swap(m_commands);
        if (include_latest_search && m_latestSearch.has_value()) {
            tasks.push_back(std::move(*m_latestSearch));
        }
        m_latestSearch.reset();
    }
    for (QueuedTask &queued : tasks) {
        try {
            queued.task(*m_context);
            if (queued.on_success) {
                PostToUi(std::move(queued.on_success));
            }
        } catch (const std::exception &error) {
            ReportError(std::move(queued.on_error), error.what());
        } catch (...) {
            ReportError(std::move(queued.on_error), "Unhandled storage worker exception");
        }
    }
}

void StorageWorker::HandleClipboardUpdate() {
    if (m_context == nullptr) {
        return;
    }

    const AppSettings before = m_context->settings;
    bool saved = false;
    try {
        saved = m_context->clipboard.OnClipboardUpdate();
    } catch (const std::exception &error) {
        ReportError({}, error.what());
    } catch (...) {
        ReportError({}, "Unhandled clipboard update exception");
    }
    if (before.ignore_events != m_context->settings.ignore_events ||
        before.ignore_only_next_event != m_context->settings.ignore_only_next_event) {
        NotifySettingsChanged(m_context->settings);
    }
    if (saved) {
        NotifyHistoryChanged();
    }
}

void StorageWorker::HandleShutdown() {
    ProcessTasks(false);
    ::PostQuitMessage(0);
}

void StorageWorker::SignalReady(StorageInitialState state) {
    {
        std::lock_guard lock(m_readyMutex);
        m_initialState = std::move(state);
        m_ready = true;
    }
    m_readyCondition.notify_one();
}

void StorageWorker::SignalStartupFailure(std::exception_ptr error) {
    {
        std::lock_guard lock(m_readyMutex);
        m_startupError = std::move(error);
        m_ready = true;
    }
    m_readyCondition.notify_one();
}

void StorageWorker::NotifyHistoryChanged() {
    UiCallback callback;
    {
        std::lock_guard lock(m_handlerMutex);
        callback = m_historyChangedHandler;
    }
    if (callback) {
        PostToUi(std::move(callback));
    }
}

void StorageWorker::NotifySettingsChanged(const AppSettings &settings) {
    SettingsChangedCallback callback;
    {
        std::lock_guard lock(m_handlerMutex);
        callback = m_settingsChangedHandler;
    }
    if (callback) {
        const AppSettings snapshot = settings;
        PostToUi([callback = std::move(callback), snapshot]() {
            callback(snapshot);
        });
    }
}
