#pragma once

#include "PlatformConfig.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "Database.h"
#include "Settings.h"

struct DatabaseContext {
    Database &database;
    AppSettings &settings;
};

struct DatabaseInitialState {
    AppSettings settings;
    bool suppress_clear_alert = false;
    std::array<std::vector<std::wstring>, 3> ignored_lists;
};

struct ClipboardQueueStats {
    std::uint64_t accepted = 0;
    std::uint64_t saved = 0;
    std::uint64_t dropped = 0;
};

class DatabaseActor {
public:
    using Task = std::function<void(DatabaseContext &)>;
    using UiCallback = std::function<void()>;
    using ErrorHandler = std::function<void(const std::string &)>;
    using SuccessHandler = std::function<void()>;
    using SettingsChangedCallback = std::function<void(const AppSettings &)>;

    explicit DatabaseActor(std::filesystem::path path);
    ~DatabaseActor();

    DatabaseActor(const DatabaseActor &) = delete;
    DatabaseActor &operator=(const DatabaseActor &) = delete;

    DatabaseInitialState Start();
    void Stop();

    void SetUiWindow(HWND window) noexcept;
    bool Post(Task task, ErrorHandler on_error = {}, SuccessHandler on_success = {});
    bool PostLatestSearch(Task task, ErrorHandler on_error = {}, SuccessHandler on_success = {});
    bool PostClipboardSnapshot(ClipboardSnapshot snapshot);
    bool PostToUi(UiCallback callback);
    void DrainUiCallbacks();

    void SetHistoryChangedHandler(UiCallback callback);
    void SetSettingsChangedHandler(SettingsChangedCallback callback);
    void SetErrorHandler(ErrorHandler callback);

    ClipboardQueueStats ClipboardStats() const noexcept;
    const std::filesystem::path &Path() const noexcept { return m_path; }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

private:
    struct QueuedTask {
        Task task;
        ErrorHandler on_error;
        SuccessHandler on_success;
    };

    using Operation = std::variant<QueuedTask, ClipboardSnapshot>;

    void ThreadMain(std::stop_token stop_token);
    bool Enqueue(QueuedTask task, bool latest_search);
    void ProcessOperations(bool include_latest_search = true);
    void HandleShutdown();
    void SignalReady(DatabaseInitialState state);
    void SignalStartupFailure(std::exception_ptr error);
    void NotifyHistoryChanged();
    void NotifySettingsChanged(const AppSettings &settings);
    void ReportError(ErrorHandler on_error, std::string message);

    static constexpr size_t kMaximumQueuedTasks = 256;
    static constexpr size_t kMaximumQueuedSnapshots = 1024;

    std::filesystem::path m_path;
    std::jthread m_thread;

    std::atomic<bool> m_accepting{false};
    std::atomic<HWND> m_workerWindow{nullptr};
    std::atomic<HWND> m_uiWindow{nullptr};

    std::mutex m_commandMutex;
    std::deque<Operation> m_operations;
    std::optional<QueuedTask> m_latestSearch;
    size_t m_queuedTasks = 0;
    size_t m_queuedSnapshots = 0;

    std::mutex m_resultMutex;
    std::deque<UiCallback> m_uiCallbacks;
    bool m_uiResultMessagePosted = false;

    std::mutex m_handlerMutex;
    UiCallback m_historyChangedHandler;
    SettingsChangedCallback m_settingsChangedHandler;
    ErrorHandler m_errorHandler;

    std::mutex m_readyMutex;
    std::condition_variable m_readyCondition;
    bool m_ready = false;
    std::exception_ptr m_startupError;
    DatabaseInitialState m_initialState;

    DatabaseContext *m_context = nullptr;
    std::atomic<bool> m_historyNotificationPending{false};
    std::atomic<std::uint64_t> m_acceptedSnapshots{0};
    std::atomic<std::uint64_t> m_savedSnapshots{0};
    std::atomic<std::uint64_t> m_droppedSnapshots{0};
};
