#include "StorageWorker.h"

#include "Constants.h"

#include <utility>
#include <stdexcept>

StorageWorker::StorageWorker(std::filesystem::path path)
    : m_path(std::move(path)) {}

StorageWorker::~StorageWorker() {
    Stop();
}

void StorageWorker::Start() {
    std::lock_guard lock(m_jobMutex);
    if (m_started) {
        return;
    }

    std::promise<void> ready;
    auto future = ready.get_future();
    m_stopping = false;
    m_thread = std::jthread(
        [this, ready = std::move(ready)](std::stop_token stop_token) mutable {
            ThreadMain(stop_token, std::move(ready));
        }
    );
    future.get();
    m_started = true;
}

void StorageWorker::Stop() {
    {
        std::lock_guard lock(m_jobMutex);
        if (!m_started && !m_thread.joinable()) {
            return;
        }
        m_stopping = true;
    }
    m_uiWindow.store(nullptr, std::memory_order_release);
    {
        std::lock_guard lock(m_searchMutex);
        m_searchStop.request_stop();
    }
    m_thread.request_stop();
    m_jobCondition.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    {
        std::lock_guard lock(m_jobMutex);
        m_jobs.clear();
        m_started = false;
    }
    {
        std::lock_guard lock(m_callbackMutex);
        m_uiCallbacks.clear();
    }
}

void StorageWorker::SetUiWindow(HWND window) noexcept {
    m_uiWindow.store(window, std::memory_order_release);
}

void StorageWorker::DrainUiCallbacks() {
    std::deque<std::function<void()>> callbacks;
    {
        std::lock_guard lock(m_callbackMutex);
        callbacks.swap(m_uiCallbacks);
    }
    for (auto &callback : callbacks) {
        if (callback) {
            callback();
        }
    }
}

void StorageWorker::Enqueue(Job job) {
    {
        std::lock_guard lock(m_jobMutex);
        if (!m_started || m_stopping) {
            throw std::runtime_error("Storage worker is not available");
        }
        m_jobs.push_back(std::move(job));
    }
    m_jobCondition.notify_one();
}

void StorageWorker::ThreadMain(std::stop_token stop_token, std::promise<void> ready) {
    try {
        Database database(m_path);
        ready.set_value();

        while (!stop_token.stop_requested()) {
            Job job;
            {
                std::unique_lock lock(m_jobMutex);
                m_jobCondition.wait(lock, [&] {
                    return stop_token.stop_requested() || !m_jobs.empty();
                });
                if (stop_token.stop_requested()) {
                    break;
                }
                job = std::move(m_jobs.front());
                m_jobs.pop_front();
            }
            if (job) {
                job(database, stop_token);
            }
        }
    } catch (...) {
        try {
            ready.set_exception(std::current_exception());
        } catch (...) {
        }
    }
}

void StorageWorker::PostUi(std::function<void()> callback) {
    const HWND window = m_uiWindow.load(std::memory_order_acquire);
    if (window == nullptr || m_stopping) {
        return;
    }
    {
        std::lock_guard lock(m_callbackMutex);
        m_uiCallbacks.push_back(std::move(callback));
    }
    ::PostMessageW(window, AppConstants::kStorageWorkerResultMessage, 0, 0);
}

AppSettings StorageWorker::LoadSettings() {
    return Invoke([](Database &database) {
        return AppSettings::Load(database);
    });
}

void StorageWorker::SaveSettings(const AppSettings &settings) {
    Invoke([&settings](Database &database) {
        settings.Save(database);
    });
}

bool StorageWorker::LoadSuppressClearAlert() {
    return Invoke([](Database &database) {
        return database.GetSetting(L"behavior.suppressClearAlert").value_or(L"0") == L"1";
    });
}

void StorageWorker::SaveSuppressClearAlert(bool enabled) {
    Invoke([enabled](Database &database) {
        database.SetSetting(L"behavior.suppressClearAlert", enabled ? L"1" : L"0");
    });
}

StorageWorker::IgnoreLists StorageWorker::LoadIgnoreLists() {
    return Invoke([](Database &database) {
        return IgnoreLists{
            database.GetList(IgnoreListKind::Applications),
            database.GetList(IgnoreListKind::Formats),
            database.GetList(IgnoreListKind::Regexps),
        };
    });
}

std::vector<std::wstring> StorageWorker::GetList(IgnoreListKind list) {
    return Invoke([list](Database &database) {
        return database.GetList(list);
    });
}

void StorageWorker::ReplaceList(IgnoreListKind list, const std::vector<std::wstring> &values) {
    Invoke([list, &values](Database &database) {
        database.ReplaceList(list, values);
    });
}

void StorageWorker::ResetIgnoredFormats() {
    Invoke([](Database &database) {
        database.ResetIgnoredFormats();
    });
}

void StorageWorker::SaveClipboardAsync(
    ClipboardSnapshot capture,
    int max_unpinned,
    Completion callback
) {
    Enqueue([this, capture = std::move(capture), max_unpinned, callback = std::move(callback)](
        Database &database,
        std::stop_token stop_token
    ) mutable {
        if (stop_token.stop_requested()) {
            return;
        }
        try {
            database.SaveClipboard(capture, max_unpinned);
            PostUi([callback = std::move(callback)]() mutable {
                if (callback) callback(true, {});
            });
        } catch (const std::exception &error) {
            PostUi([callback = std::move(callback), message = std::string(error.what())]() mutable {
                if (callback) callback(false, message);
            });
        } catch (...) {
            PostUi([callback = std::move(callback)]() mutable {
                if (callback) callback(false, "Unable to save clipboard history");
            });
        }
    });
}

void StorageWorker::SearchHistoryAsync(
    std::wstring query,
    int search_mode,
    int sort_by,
    bool pins_at_bottom,
    std::uint64_t generation,
    SearchCallback callback
) {
    std::stop_token request_stop;
    {
        std::lock_guard lock(m_searchMutex);
        m_searchStop.request_stop();
        m_searchStop = std::stop_source();
        request_stop = m_searchStop.get_token();
    }

    Enqueue([this, query = std::move(query), search_mode, sort_by, pins_at_bottom,
             generation, callback = std::move(callback), request_stop](
        Database &database,
        std::stop_token worker_stop
    ) mutable {
        const auto cancelled = [&] {
            return worker_stop.stop_requested() || request_stop.stop_requested();
        };
        if (cancelled()) {
            return;
        }
        try {
            auto items = database.SearchHistory(
                query,
                search_mode,
                sort_by,
                pins_at_bottom,
                cancelled
            );
            if (cancelled()) {
                return;
            }
            PostUi([callback = std::move(callback), generation, items = std::move(items)]() mutable {
                if (callback) callback(generation, std::move(items), {});
            });
        } catch (const std::exception &error) {
            PostUi([callback = std::move(callback), generation, message = std::string(error.what())]() mutable {
                if (callback) callback(generation, {}, message);
            });
        } catch (...) {
            PostUi([callback = std::move(callback), generation]() mutable {
                if (callback) callback(generation, {}, "Unable to search clipboard history");
            });
        }
    });
}

void StorageWorker::GetItemAsync(sqlite3_int64 id, PayloadMode mode, ItemCallback callback) {
    Enqueue([this, id, mode, callback = std::move(callback)](
        Database &database,
        std::stop_token stop_token
    ) mutable {
        if (stop_token.stop_requested()) {
            return;
        }
        try {
            auto item = database.GetItem(id, mode);
            PostUi([callback = std::move(callback), item = std::move(item)]() mutable {
                if (callback) callback(std::move(item), {});
            });
        } catch (const std::exception &error) {
            PostUi([callback = std::move(callback), message = std::string(error.what())]() mutable {
                if (callback) callback(std::nullopt, message);
            });
        } catch (...) {
            PostUi([callback = std::move(callback)]() mutable {
                if (callback) callback(std::nullopt, "Unable to load clipboard item");
            });
        }
    });
}

std::optional<ClipboardItem> StorageWorker::GetItem(sqlite3_int64 id, PayloadMode mode) {
    return Invoke([id, mode](Database &database) {
        return database.GetItem(id, mode);
    });
}

std::vector<ClipboardItem> StorageWorker::GetPinnedItems(PayloadMode mode) {
    return Invoke([mode](Database &database) {
        return database.GetPinnedItems(mode);
    });
}

void StorageWorker::MarkCopied(sqlite3_int64 id) {
    Invoke([id](Database &database) { database.MarkCopied(id); });
}

void StorageWorker::DeleteItem(sqlite3_int64 id) {
    Invoke([id](Database &database) { database.DeleteItem(id); });
}

void StorageWorker::DeleteUnpinned() {
    Invoke([](Database &database) { database.DeleteUnpinned(); });
}

void StorageWorker::DeleteAll() {
    Invoke([](Database &database) { database.DeleteAll(); });
}

void StorageWorker::TogglePin(sqlite3_int64 id, std::wstring_view pin_key, bool pinned) {
    const std::wstring key(pin_key);
    Invoke([id, key = std::move(key), pinned](Database &database) {
        database.TogglePin(id, key, pinned);
    });
}

void StorageWorker::UpdatePinnedItem(
    sqlite3_int64 id,
    std::wstring_view pin_key,
    std::wstring_view title,
    std::wstring_view text
) {
    const std::wstring key(pin_key);
    const std::wstring owned_title(title);
    const std::wstring owned_text(text);
    Invoke([id, key = std::move(key), owned_title = std::move(owned_title),
            owned_text = std::move(owned_text)](Database &database) {
        database.UpdatePinnedItem(id, key, owned_title, owned_text);
    });
}

void StorageWorker::UpdatePinnedMetadata(
    sqlite3_int64 id,
    std::wstring_view pin_key,
    std::wstring_view title
) {
    const std::wstring key(pin_key);
    const std::wstring owned_title(title);
    Invoke([id, key = std::move(key), owned_title = std::move(owned_title)](Database &database) {
        database.UpdatePinnedMetadata(id, key, owned_title);
    });
}

void StorageWorker::RegenerateTitles(bool show_special_symbols) {
    Invoke([show_special_symbols](Database &database) {
        database.RegenerateTitles(show_special_symbols);
    });
}

void StorageWorker::TrimUnpinned(int max_unpinned) {
    Invoke([max_unpinned](Database &database) { database.TrimUnpinned(max_unpinned); });
}

sqlite3_int64 StorageWorker::CountItems() {
    return Invoke([](Database &database) { return database.CountItems(); });
}

std::uintmax_t StorageWorker::StorageBytes() {
    return Invoke([](Database &database) { return database.StorageBytes(); });
}
