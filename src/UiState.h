#pragma once

#include <cstdint>
#include <string>

#include <sqlite3.h>

// The state that can be observed by more than one window procedure lives in
// one place. A pending bitmask is deliberately part of the state so multiple
// producers can coalesce into one owner-window message.
struct UiState {
    std::wstring historyQuery;
    sqlite3_int64 selectedItemId = 0;
    sqlite3_int64 previewCandidateId = 0;
    sqlite3_int64 previewItemId = 0;
    bool previewSuppressed = false;
    bool popupVisible = false;
    std::uint32_t pendingUpdates = 0;
    bool updateMessagePosted = false;
};
