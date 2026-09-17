#include "DatabaseActor.h"

#include "Constants.h"

#include <windows.h>

#include <stdexcept>
#include <utility>

namespace {

constexpr wchar_t kDatabaseActorWindowClass[] = L"maccy.DatabaseActorWindow";

void RegisterDatabaseActorWindowClass() {
    WNDCLASSEXW window_class{sizeof(window_class)};
    window_class.lpfnWndProc = &DatabaseActor::WindowProc;
    window_class.hInstance = ::GetModuleHandleW(nullptr);
    window_class.lpszClassName = kDatabaseActorWindowClass;
    if (::RegisterClassExW(&window_class) == 0 && ::GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        throw std::runtime_error("Unable to register database actor window class");
    }
}

bool ClipboardSettingsChanged(const AppSettings &before, const AppSettings &after) {
    return before.ignore_events != after.ignore_events ||
        before.ignore_only_next_event != after.ignore_only_next_event;
}

} // namespace

DatabaseActor::DatabaseActor(std::filesystem::path path)
    : m_path(std::move(path)) {}

DatabaseActor::~DatabaseActor() {
    Stop();
}

DatabaseInitialState DatabaseActor::Start() {
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
    m_historyNotificationPending.store(false, std::memory_order_release);
    m_acceptedSnapshots.store(0, std::memory_order_release);
    m_savedSnapshots.store(0, std::memory_order_release);
    m_droppedSnapshots.store(0, std::memory_order_release);

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

void DatabaseActor::Stop() {
    if (!m_thread.joinable()) {
        return;
    }

    m_accepting.store(false, std::memory_order_release);
    m_thread.request_stop();
    if (const HWND window = m_workerWindow.load(std::memory_order_acquire);
        window != nullptr) {
        ::PostMessageW(window, AppConstants::kDatabaseActorShutdownMessage, 0, 0);
    }
    m_thread.join();
    m_workerWindow.store(nullptr, std::memory_order_release);

    {
        std::lock_guard lock(m_commandMutex);
        m_operations.clear();
        m_latestSearch.reset();
        m_queuedTasks = 0;
        m_queuedSnapshots = 0;
    }
    {
        std::lock_guard lock(m_resultMutex);
        m_uiCallbacks.clear();
        m_uiResultMessagePosted = false;
    }
    m_historyNotificationPending.store(false, std::memory_order_release);
}

void DatabaseActor::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
}

bool DatabaseActor::Enqueue(QueuedTask task, bool latest_search) {
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
        if (::PostMessageW(window, AppConstants::kDatabaseActorCommandMessage, 0, 0) != FALSE) {
            return true;
        }
        m_latestSearch = std::move(previous);
        return false;
    }

    if (m_queuedTasks >= kMaximumQueuedTasks) {
        return false;
    }
    m_operations.emplace_back(std::move(task));
    ++m_queuedTasks;
    if (::PostMessageW(window, AppConstants::kDatabaseActorCommandMessage, 0, 0) != FALSE) {
        return true;
    }
    m_operations.pop_back();
    --m_queuedTasks;
    return false;
}

bool DatabaseActor::Post(
    Task task,
    ErrorHandler on_error,
    SuccessHandler on_success
) {
    return Enqueue(
        QueuedTask{std::move(task), std::move(on_error), std::move(on_success)},
        false
    );
}

bool DatabaseActor::PostLatestSearch(
    Task task,
    ErrorHandler on_error,
    SuccessHandler on_success
) {
    return Enqueue(
        QueuedTask{std::move(task), std::move(on_error), std::move(on_success)},
        true
    );
}

bool DatabaseActor::PostClipboardSnapshot(ClipboardSnapshot snapshot) {
    if (!m_accepting.load(std::memory_order_acquire)) {
        m_droppedSnapshots.fetch_add(1, std::memory_order_acq_rel);
        return false;
    }

    std::lock_guard lock(m_commandMutex);
    if (!m_accepting.load(std::memory_order_relaxed) ||
        m_queuedSnapshots >= kMaximumQueuedSnapshots) {
        m_droppedSnapshots.fetch_add(1, std::memory_order_acq_rel);
        return false;
    }

    const HWND window = m_workerWindow.load(std::memory_order_acquire);
    if (window == nullptr) {
        m_droppedSnapshots.fetch_add(1, std::memory_order_acq_rel);
        return false;
    }

    m_operations.emplace_back(std::move(snapshot));
    ++m_queuedSnapshots;
    m_acceptedSnapshots.fetch_add(1, std::memory_order_acq_rel);
    if (::PostMessageW(window, AppConstants::kDatabaseActorCommandMessage, 0, 0) != FALSE) {
        return true;
    }
    m_operations.pop_back();
    --m_queuedSnapshots;
    m_acceptedSnapshots.fetch_sub(1, std::memory_order_acq_rel);
    m_droppedSnapshots.fetch_add(1, std::memory_order_acq_rel);
    return false;
}

bool DatabaseActor::PostToUi(UiCallback callback) {
    if (!callback) {
        return false;
    }
    const HWND window = m_uiWindow.load(std::memory_order_acquire);
    if (window == nullptr) {
        return false;
    }

    std::lock_guard lock(m_resultMutex);
    m_uiCallbacks.push_back(std::move(callback));
    if (m_uiResultMessagePosted) {
        return true;
    }
    m_uiResultMessagePosted = true;
    if (::PostMessageW(window, AppConstants::kDatabaseActorResultMessage, 0, 0)) {
        return true;
    }
    m_uiResultMessagePosted = false;
    m_uiCallbacks.pop_back();
    return false;
}

void DatabaseActor::SetErrorHandler(ErrorHandler callback) {
    std::lock_guard lock(m_handlerMutex);
    m_errorHandler = std::move(callback);
}

void DatabaseActor::ReportError(ErrorHandler on_error, std::string message) {
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

void DatabaseActor::DrainUiCallbacks() {
    std::deque<UiCallback> callbacks;
    {
        std::lock_guard lock(m_resultMutex);
        callbacks.swap(m_uiCallbacks);
        m_uiResultMessagePosted = false;
    }
    for (UiCallback &callback : callbacks) {
        if (callback) {
            callback();
        }
    }
}

void DatabaseActor::SetHistoryChangedHandler(UiCallback callback) {
    std::lock_guard lock(m_handlerMutex);
    m_historyChangedHandler = std::move(callback);
}

void DatabaseActor::SetSettingsChangedHandler(SettingsChangedCallback callback) {
    std::lock_guard lock(m_handlerMutex);
    m_settingsChangedHandler = std::move(callback);
}

ClipboardQueueStats DatabaseActor::ClipboardStats() const noexcept {
    return ClipboardQueueStats{
        m_acceptedSnapshots.load(std::memory_order_acquire),
        m_savedSnapshots.load(std::memory_order_acquire),
        m_droppedSnapshots.load(std::memory_order_acquire),
    };
}

LRESULT CALLBACK DatabaseActor::WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    auto *actor = reinterpret_cast<DatabaseActor *>(::GetWindowLongPtrW(
        window,
        GWLP_USERDATA
    ));
    if (message == WM_NCCREATE) {
        const auto *create = reinterpret_cast<const CREATESTRUCTW *>(lParam);
        actor = static_cast<DatabaseActor *>(create->lpCreateParams);
        ::SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(actor));
    }
    if (actor == nullptr) {
        return ::DefWindowProcW(window, message, wParam, lParam);
    }

    switch (message) {
    case AppConstants::kDatabaseActorCommandMessage:
        actor->ProcessOperations();
        return 0;
    case AppConstants::kDatabaseActorShutdownMessage:
        actor->HandleShutdown();
        return 0;
    default:
        break;
    }
    return ::DefWindowProcW(window, message, wParam, lParam);
}

void DatabaseActor::ThreadMain(std::stop_token stop_token) {
    const std::stop_callback wake_on_stop(stop_token, [this] {
        if (const HWND window = m_workerWindow.load(std::memory_order_acquire);
            window != nullptr) {
            ::PostMessageW(window, AppConstants::kDatabaseActorShutdownMessage, 0, 0);
        }
    });
    try {
        RegisterDatabaseActorWindowClass();
        const HWND window = ::CreateWindowExW(
            0,
            kDatabaseActorWindowClass,
            L"maccy database actor",
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
            throw std::runtime_error("Unable to create database actor window");
        }
        m_workerWindow.store(window, std::memory_order_release);

        Database database(m_path);
        AppSettings settings = AppSettings::Load(database);

        DatabaseInitialState initial;
        initial.settings = settings;
        initial.suppress_clear_alert =
            database.GetSetting(L"behavior.suppressClearAlert") == std::optional<std::wstring>(L"1");
        initial.ignored_lists[0] = database.GetList(DatabaseList::IgnoredApplications);
        initial.ignored_lists[1] = database.GetList(DatabaseList::IgnoredFormats);
        initial.ignored_lists[2] = database.GetList(DatabaseList::IgnoredRegexps);

        DatabaseContext context{database, settings};
        m_context = &context;
        SignalReady(std::move(initial));
        if (stop_token.stop_requested()) {
            ::PostMessageW(window, AppConstants::kDatabaseActorShutdownMessage, 0, 0);
        }

        MSG message{};
        for (;;) {
            const BOOL result = ::GetMessageW(&message, nullptr, 0, 0);
            if (result <= 0) {
                break;
            }
            ::TranslateMessage(&message);
            ::DispatchMessageW(&message);
        }

        m_context = nullptr;
        ::DestroyWindow(window);
        m_workerWindow.store(nullptr, std::memory_order_release);
    } catch (...) {
        SignalStartupFailure(std::current_exception());
    }
}

void DatabaseActor::ProcessOperations(bool include_latest_search) {
    if (m_context == nullptr) {
        return;
    }

    std::deque<Operation> operations;
    {
        std::lock_guard lock(m_commandMutex);
        operations.swap(m_operations);
        m_queuedTasks = 0;
        m_queuedSnapshots = 0;
        if (include_latest_search && m_latestSearch.has_value()) {
            operations.emplace_back(std::move(*m_latestSearch));
        }
        m_latestSearch.reset();
    }

    for (Operation &operation : operations) {
        if (auto *task = std::get_if<QueuedTask>(&operation)) {
            const AppSettings before = m_context->settings;
            try {
                task->task(*m_context);
                if (ClipboardSettingsChanged(before, m_context->settings)) {
                    NotifySettingsChanged(m_context->settings);
                }
                if (task->on_success) {
                    PostToUi(std::move(task->on_success));
                }
            } catch (const std::exception &error) {
                ReportError(std::move(task->on_error), error.what());
            } catch (...) {
                ReportError(std::move(task->on_error), "Unhandled database actor exception");
            }
            continue;
        }

        auto *snapshot = std::get_if<ClipboardSnapshot>(&operation);
        if (snapshot == nullptr) {
            continue;
        }
        try {
            m_context->database.SaveClipboard(*snapshot, m_context->settings.history_size);
            m_savedSnapshots.fetch_add(1, std::memory_order_acq_rel);
            NotifyHistoryChanged();
        } catch (const std::exception &error) {
            ReportError({}, error.what());
        } catch (...) {
            ReportError({}, "Unhandled clipboard snapshot save exception");
        }
    }
}

void DatabaseActor::HandleShutdown() {
    ProcessOperations(false);
    ::PostQuitMessage(0);
}

void DatabaseActor::SignalReady(DatabaseInitialState state) {
    {
        std::lock_guard lock(m_readyMutex);
        m_initialState = std::move(state);
        m_ready = true;
    }
    m_readyCondition.notify_one();
}

void DatabaseActor::SignalStartupFailure(std::exception_ptr error) {
    {
        std::lock_guard lock(m_readyMutex);
        m_startupError = std::move(error);
        m_ready = true;
    }
    m_readyCondition.notify_one();
}

void DatabaseActor::NotifyHistoryChanged() {
    if (m_historyNotificationPending.exchange(true, std::memory_order_acq_rel)) {
        return;
    }

    UiCallback callback;
    {
        std::lock_guard lock(m_handlerMutex);
        callback = m_historyChangedHandler;
    }
    if (!callback) {
        m_historyNotificationPending.store(false, std::memory_order_release);
        return;
    }

    if (!PostToUi([this, callback = std::move(callback)]() mutable {
        m_historyNotificationPending.store(false, std::memory_order_release);
        callback();
    })) {
        m_historyNotificationPending.store(false, std::memory_order_release);
    }
}

void DatabaseActor::NotifySettingsChanged(const AppSettings &settings) {
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
