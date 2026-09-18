#pragma once

#include "ClipboardData.h"
#include "Database.h"
#include "Settings.h"

#include <windows.h>

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <future>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// Owns the application's writable SQLite connection.  Callers never touch
// SQLite directly; synchronous calls still execute on this worker thread.
class StorageWorker {
public:
    using IgnoreLists = std::array<std::vector<std::wstring>, 3>;
    using SearchCallback = std::function<void(
        std::uint64_t generation,
        std::vector<ClipboardItem> items,
        std::string error
    )>;
    using ItemCallback = std::function<void(
        std::optional<ClipboardItem> item,
        std::string error
    )>;
    using Completion = std::function<void(bool success, std::string error)>;

    explicit StorageWorker(std::filesystem::path path);
    ~StorageWorker();

    StorageWorker(const StorageWorker &) = delete;
    StorageWorker &operator=(const StorageWorker &) = delete;

    void Start();
    void Stop();
    void SetUiWindow(HWND window) noexcept;
    void DrainUiCallbacks();

    const std::filesystem::path &Path() const noexcept { return m_path; }

    AppSettings LoadSettings();
    void SaveSettings(const AppSettings &settings);
    bool LoadSuppressClearAlert();
    void SaveSuppressClearAlert(bool enabled);
    IgnoreLists LoadIgnoreLists();

    std::vector<std::wstring> GetList(IgnoreListKind list);
    void ReplaceList(IgnoreListKind list, const std::vector<std::wstring> &values);
    void ResetIgnoredFormats();

    void SaveClipboardAsync(ClipboardSnapshot capture, int max_unpinned, Completion callback);
    void SearchHistoryAsync(
        std::wstring query,
        int search_mode,
        int sort_by,
        bool pins_at_bottom,
        std::uint64_t generation,
        SearchCallback callback
    );
    void GetItemAsync(sqlite3_int64 id, PayloadMode mode, ItemCallback callback);

    std::optional<ClipboardItem> GetItem(sqlite3_int64 id, PayloadMode mode = PayloadMode::Metadata);
    std::vector<ClipboardItem> GetPinnedItems(PayloadMode mode = PayloadMode::Metadata);
    void MarkCopied(sqlite3_int64 id);
    void DeleteItem(sqlite3_int64 id);
    void DeleteUnpinned();
    void DeleteAll();
    void TogglePin(sqlite3_int64 id, std::wstring_view pin_key, bool pinned);
    void UpdatePinnedItem(
        sqlite3_int64 id,
        std::wstring_view pin_key,
        std::wstring_view title,
        std::wstring_view text
    );
    void UpdatePinnedMetadata(sqlite3_int64 id, std::wstring_view pin_key, std::wstring_view title);
    void RegenerateTitles(bool show_special_symbols);
    void TrimUnpinned(int max_unpinned);
    sqlite3_int64 CountItems();
    std::uintmax_t StorageBytes();

private:
    using Job = std::function<void(Database &, std::stop_token)>;

    void Enqueue(Job job);
    void ThreadMain(std::stop_token stop_token, std::promise<void> ready);
    void PostUi(std::function<void()> callback);

    template <typename Function>
    auto Invoke(Function function) -> std::invoke_result_t<Function, Database &> {
        using Result = std::invoke_result_t<Function, Database &>;
        auto promise = std::make_shared<std::promise<Result>>();
        auto future = promise->get_future();
        Enqueue([function = std::move(function), promise](Database &database, std::stop_token) mutable {
            try {
                if constexpr (std::is_void_v<Result>) {
                    std::invoke(function, database);
                    promise->set_value();
                } else {
                    promise->set_value(std::invoke(function, database));
                }
            } catch (...) {
                promise->set_exception(std::current_exception());
            }
        });
        return future.get();
    }

    std::filesystem::path m_path;
    std::jthread m_thread;
    std::mutex m_jobMutex;
    std::condition_variable m_jobCondition;
    std::deque<Job> m_jobs;
    bool m_started = false;
    std::atomic<bool> m_stopping{false};

    std::atomic<HWND> m_uiWindow{nullptr};
    std::mutex m_callbackMutex;
    std::deque<std::function<void()>> m_uiCallbacks;

    std::mutex m_searchMutex;
    std::stop_source m_searchStop;
};
