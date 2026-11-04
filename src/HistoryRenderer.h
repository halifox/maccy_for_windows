#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <list>
#include <mutex>
#include <optional>
#include <regex>
#include <stop_token>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Win32Resources.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlctrls.h>
#include <atlgdi.h>

#include "ClipboardData.h"
#include "SearchHeaderLayout.h"
#include "Settings.h"
#include "PreviewDecoder.h"

// History list rendering and drawing component
class HistoryRenderer {
public:
    explicit HistoryRenderer(AppSettings& settings);
    ~HistoryRenderer();

    HistoryRenderer(const HistoryRenderer&) = delete;
    HistoryRenderer& operator=(const HistoryRenderer&) = delete;

    // Lifecycle
    bool Initialize(CWindow owner);
    bool UpdateFonts(UINT dpi);
    void Shutdown();
    void PrepareHistory(const std::vector<ClipboardItem> &items);
    void SetImageThumbnails(std::unordered_map<sqlite3_int64, PreviewBitmap> thumbnails);
    void SetImageThumbnail(sqlite3_int64 id, PreviewBitmap thumbnail);
    const PreviewBitmap* ImageThumbnail(sqlite3_int64 id) const noexcept;
    void ClearImageThumbnails() noexcept;
    bool ApplyLoadedIcons();
    // Drawing operations
    void DrawHistoryItem(DRAWITEMSTRUCT* draw,
                         const std::vector<ClipboardItem>& items,
                         int activeIndex,
                         int activeFooter,
                         std::wstring_view searchQuery);
    void DrawMenuButton(DRAWITEMSTRUCT* draw,
                        int activeFooter,
                        const std::array<CButton, AppConstants::UI::kFooterButtonCount>& footerButtons);
    void OnPaint(HDC dc, const RECT& client,
                 int pinSeparatorY,
                 int footerSeparatorY,
                 const SearchHeaderLayout::Geometry& header,
                 bool showSearchClear);

    // Font access
    HFONT GetNormalFont() const { return m_normalFont; }
    HFONT GetSmallFont() const { return m_smallFont; }
    HFONT GetBoldFont() const { return m_boldFont; }

    // Text utilities
    std::wstring DisplayText(const ClipboardItem& item) const;

private:
    struct HistoryItemLayout {
        RECT background{};
        RECT icon{};
        RECT attachment{};
        RECT swatch{};
        RECT content{};
        RECT shortcut{};
    };

    // Layout
    HistoryItemLayout LayoutHistoryItem(const RECT& row, const ClipboardItem& item,
                                        bool has_color_swatch, UINT dpi) const;

    struct FittedLine {
        std::wstring text;
        std::vector<std::pair<size_t, size_t>> ranges;
        int width = -1;
        std::wstring query;
    };
    struct IconJob {
        std::wstring key;
        std::wstring path;
    };
    struct IconResult {
        std::wstring key;
        UniqueIcon icon;
    };

    // Drawing helpers
    void DrawTextWithHighlights(HDC dc, RECT rect, sqlite3_int64 item_id, std::wstring_view text,
                                bool selected, std::wstring_view searchQuery);
    const FittedLine &FittedLineFor(sqlite3_int64 item_id, std::wstring_view text, int width,
                                    std::wstring_view search_query, HDC dc);
    void QueueIcon(std::wstring key, std::wstring path);
    void IconThreadMain(std::stop_token stop_token);
    static UniqueIcon LoadShellIcon(const std::wstring &path);
    std::vector<std::pair<size_t, size_t>> HighlightRanges(std::wstring_view text,
                                                            std::wstring_view searchQuery);
    void PrepareHighlightPattern(std::wstring_view searchQuery);
    HFONT FontForHighlight(HighlightMatch match) const;

    // Icon management
    HICON IconForPath(std::wstring_view path);

    struct IconCacheEntry {
        UniqueIcon icon;
        std::list<std::wstring>::iterator lru;
    };

    AppSettings& m_settings;
    HWND m_owner = nullptr;

    CFont m_normalFont;
    CFont m_smallFont;
    CFont m_boldFont;
    CFont m_italicFont;
    CFont m_underlineFont;

    std::unordered_map<std::wstring, IconCacheEntry> m_iconCache;
    std::list<std::wstring> m_iconLru;
    std::unordered_set<std::wstring> m_iconQueued;
    std::mutex m_iconMutex;
    std::condition_variable m_iconCv;
    std::deque<IconJob> m_iconJobs;
    std::mutex m_iconResultMutex;
    std::deque<IconResult> m_iconResults;
    std::atomic<bool> m_iconPosted{false};
    std::jthread m_iconThread;
    std::unordered_map<sqlite3_int64, FittedLine> m_fittedLines;
    std::unordered_map<sqlite3_int64, PreviewBitmap> m_imageThumbnails;
    std::vector<int> m_unpinnedShortcutNumbers;
    std::wstring m_highlightQuery;
    SearchMode m_highlightMode = SearchMode::Exact;
    std::optional<std::wregex> m_highlightRegex;
    bool m_highlightPatternReady = false;
};
