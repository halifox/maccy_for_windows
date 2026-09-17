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
#include <vector>

#include "Database.h"
#include "Settings.h"

class ClipboardMonitor;

struct StorageContext {
    Database &database;
    ClipboardMonitor &clipboard;
    AppSettings &settings;
};

struct StorageInitialState {
    AppSettings settings;
    bool suppress_clear_alert = false;
    std::array<std::vector<std::wstring>, 3> ignored_lists;
};

class StorageWorker {
public:
    using Task = std::function<void(StorageContext &)>;
    using UiCallback = std::function<void()>;
    using ErrorHandler = std::function<void(const std::string &)>;
    using SuccessHandler = std::function<void()>;
    using SettingsChangedCallback = std::function<void(const AppSettings &)>;

    explicit StorageWorker(std::filesystem::path path, bool monitor_clipboard = true);
    ~StorageWorker();

    StorageWorker(const StorageWorker &) = delete;
    StorageWorker &operator=(const StorageWorker &) = delete;

    StorageInitialState Start();
    void Stop();

    void SetUiWindow(HWND window) noexcept;
    bool Post(Task task, ErrorHandler on_error = {}, SuccessHandler on_success = {});
    bool PostLatestSearch(Task task, ErrorHandler on_error = {}, SuccessHandler on_success = {});
    bool PostToUi(UiCallback callback);
    void DrainUiCallbacks();

    void SetHistoryChangedHandler(UiCallback callback);
    void SetSettingsChangedHandler(SettingsChangedCallback callback);
    void SetErrorHandler(ErrorHandler callback);

    const std::filesystem::path &Path() const noexcept { return m_path; }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

private:
    struct QueuedTask {
        Task task;
        ErrorHandler on_error;
        SuccessHandler on_success;
    };

    void ThreadMain(std::stop_token stop_token);
    bool Enqueue(QueuedTask task, bool latest_search);
    void ProcessTasks(bool include_latest_search = true);
    void HandleClipboardUpdate();
    void HandleShutdown();
    void SignalReady(StorageInitialState state);
    void SignalStartupFailure(std::exception_ptr error);
    void NotifyHistoryChanged();
    void NotifySettingsChanged(const AppSettings &settings);
    void ReportError(ErrorHandler on_error, std::string message);

    std::filesystem::path m_path;
    bool m_monitorClipboard = true;
    std::jthread m_thread;

    std::atomic<bool> m_accepting{false};
    std::atomic<HWND> m_workerWindow{nullptr};
    std::atomic<HWND> m_uiWindow{nullptr};

    std::mutex m_commandMutex;
    std::deque<QueuedTask> m_commands;
    std::optional<QueuedTask> m_latestSearch;

    std::mutex m_resultMutex;
    std::deque<UiCallback> m_uiCallbacks;

    std::mutex m_handlerMutex;
    UiCallback m_historyChangedHandler;
    SettingsChangedCallback m_settingsChangedHandler;
    ErrorHandler m_errorHandler;

    std::mutex m_readyMutex;
    std::condition_variable m_readyCondition;
    bool m_ready = false;
    std::exception_ptr m_startupError;
    StorageInitialState m_initialState;

    StorageContext *m_context = nullptr;
};
