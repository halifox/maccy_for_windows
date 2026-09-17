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
    using SettingsChangedCallback = std::function<void(const AppSettings &)>;

    explicit StorageWorker(std::filesystem::path path, bool monitor_clipboard = true);
    ~StorageWorker();

    StorageWorker(const StorageWorker &) = delete;
    StorageWorker &operator=(const StorageWorker &) = delete;

    StorageInitialState Start();
    void Stop();

    void SetUiWindow(HWND window) noexcept;
    bool Post(Task task);
    void PostToUi(UiCallback callback);
    void DrainUiCallbacks();

    void SetHistoryChangedHandler(UiCallback callback);
    void SetSettingsChangedHandler(SettingsChangedCallback callback);

    const std::filesystem::path &Path() const noexcept { return m_path; }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

private:
    void ThreadMain(std::stop_token stop_token);
    void ProcessTasks();
    void HandleClipboardUpdate();
    void HandleShutdown();
    void SignalReady(StorageInitialState state);
    void SignalStartupFailure(std::exception_ptr error);
    void NotifyHistoryChanged();
    void NotifySettingsChanged(const AppSettings &settings);

    std::filesystem::path m_path;
    bool m_monitorClipboard = true;
    std::jthread m_thread;

    std::atomic<bool> m_accepting{false};
    std::atomic<HWND> m_workerWindow{nullptr};
    std::atomic<HWND> m_uiWindow{nullptr};

    std::mutex m_commandMutex;
    std::deque<Task> m_commands;

    std::mutex m_resultMutex;
    std::deque<UiCallback> m_uiCallbacks;

    std::mutex m_handlerMutex;
    UiCallback m_historyChangedHandler;
    SettingsChangedCallback m_settingsChangedHandler;

    std::mutex m_readyMutex;
    std::condition_variable m_readyCondition;
    bool m_ready = false;
    std::exception_ptr m_startupError;
    StorageInitialState m_initialState;

    StorageContext *m_context = nullptr;
};
