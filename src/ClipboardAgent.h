#pragma once

#include "PlatformConfig.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <exception>
#include <functional>
#include <mutex>
#include <optional>
#include <regex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

#include "Database.h"
#include "Settings.h"

struct ClipboardAgentSettings {
    bool respect_windows_clipboard_history_markers = true;
    bool ignore_all_apps_except_listed = false;
    bool ignore_events = false;
    bool ignore_only_next_event = false;
    bool save_files = true;
    bool save_images = true;
    bool save_text = true;
    bool show_special_symbols = true;

    static ClipboardAgentSettings FromAppSettings(const AppSettings &settings);
};

class ClipboardAgent {
public:
    using SnapshotHandler = std::function<bool(ClipboardSnapshot)>;
    using SettingsChangedHandler = std::function<void(const ClipboardAgentSettings &)>;
    using OperationCallback = std::function<void(bool, std::string)>;

    ClipboardAgent(
        SnapshotHandler on_snapshot,
        SettingsChangedHandler on_settings_changed = {}
    );
    ~ClipboardAgent();

    ClipboardAgent(const ClipboardAgent &) = delete;
    ClipboardAgent &operator=(const ClipboardAgent &) = delete;

    void Start(
        ClipboardAgentSettings settings,
        std::array<std::vector<std::wstring>, 3> ignored_lists
    );
    void Stop();

    bool SetSettings(ClipboardAgentSettings settings);
    bool SetIgnoreLists(std::array<std::vector<std::wstring>, 3> ignored_lists);
    bool WriteItem(
        ClipboardItem item,
        bool remove_formatting,
        OperationCallback callback
    );
    bool ClearClipboard(OperationCallback callback = {});

    std::uint64_t DroppedSnapshots() const noexcept {
        return m_droppedSnapshots.load(std::memory_order_acquire);
    }

    static LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

private:
    using Command = std::function<void()>;

    void ThreadMain(std::stop_token stop_token);
    bool Enqueue(Command command);
    void ProcessCommands();
    void HandleClipboardUpdate();
    void HandleShutdown();
    void SignalReady();
    void SignalStartupFailure(std::exception_ptr error);

    void ApplySettings(ClipboardAgentSettings settings);
    void ApplyIgnoreLists(std::array<std::vector<std::wstring>, 3> ignored_lists);
    void NotifySettingsChanged();
    bool ReadClipboardSnapshot();
    bool WriteClipboardItem(
        const ClipboardItem &item,
        bool remove_formatting,
        std::string &error
    );
    bool ClearSystemClipboard(std::string &error);

    std::optional<ClipboardSnapshot> CaptureClipboard() const;
    bool ShouldIgnoreApplication(std::wstring_view application) const;
    bool ShouldIgnoreFormat(std::wstring_view format) const;
    bool ShouldIgnoreText(std::wstring_view text) const;

    static std::wstring ExtractClipboardText();
    static std::wstring ExtractClipboardAnsiText();
    static std::wstring FilesPreview(HGLOBAL data);
    static std::wstring HashSnapshot(const std::vector<ClipboardFormatData> &data);
    static std::wstring GetSourceApplication();
    static bool MatchesApplication(std::wstring_view actual, std::wstring_view configured);

    static constexpr size_t kMaximumQueuedCommands = 256;

    SnapshotHandler m_onSnapshot;
    SettingsChangedHandler m_onSettingsChanged;
    std::jthread m_thread;

    std::atomic<bool> m_accepting{false};
    std::atomic<HWND> m_workerWindow{nullptr};

    std::mutex m_commandMutex;
    std::deque<Command> m_commands;

    std::mutex m_readyMutex;
    std::condition_variable m_readyCondition;
    bool m_ready = false;
    std::exception_ptr m_startupError;

    ClipboardAgentSettings m_initialSettings;
    std::array<std::vector<std::wstring>, 3> m_initialIgnoredLists;

    ClipboardAgentSettings m_settings;
    std::vector<std::wstring> m_ignoredApps;
    std::vector<std::wstring> m_ignoredFormats;
    std::vector<std::wregex> m_ignoredRegexpPatterns;
    HWND m_owner = nullptr;
    bool m_clipboardListenerAdded = false;
    bool m_shuttingDown = false;
    std::optional<std::wstring> m_expectedClipboardFingerprint;

    std::atomic<std::uint64_t> m_droppedSnapshots{0};
};
