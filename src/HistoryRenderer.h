#pragma once

#include "PlatformConfig.h"
#include "Constants.h"

#include <list>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "ClipboardData.h"
#include "SearchHeaderLayout.h"
#include "Settings.h"

// History list rendering and drawing component
class HistoryRenderer {
public:
    explicit HistoryRenderer(AppSettings& settings);
    ~HistoryRenderer();

    HistoryRenderer(const HistoryRenderer&) = delete;
    HistoryRenderer& operator=(const HistoryRenderer&) = delete;

    // Lifecycle
    bool Initialize(HWND owner);
    bool UpdateFonts(UINT dpi);
    void Shutdown();
    void PrepareHistory(const std::vector<ClipboardItem> &items);

    // Drawing operations
    void DrawHistoryItem(DRAWITEMSTRUCT* draw,
                         const std::vector<ClipboardItem>& items,
                         int activeIndex,
                         int activeFooter,
                         std::wstring_view searchQuery);
    void DrawMenuButton(DRAWITEMSTRUCT* draw,
                        int activeFooter,
                        const std::array<HWND, AppConstants::UI::kFooterButtonCount>& footerButtons);
    void OnPaint(HDC dc, const RECT& client,
                 int pinSeparatorY,
                 int footerSeparatorY,
                 const SearchHeaderLayout::Geometry& header);

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
                                        bool has_color_swatch) const;

    // Drawing helpers
    void DrawTextWithHighlights(HDC dc, RECT rect, std::wstring_view text,
                                bool selected, std::wstring_view searchQuery);
    std::vector<std::pair<size_t, size_t>> HighlightRanges(std::wstring_view text,
                                                            std::wstring_view searchQuery);
    void PrepareHighlightPattern(std::wstring_view searchQuery);
    HFONT FontForHighlight(HighlightMatch match) const;

    // Icon management
    HICON IconForApplication(std::wstring_view application);

    struct IconCacheEntry {
        HICON icon = nullptr;
        std::list<std::wstring>::iterator lru;
    };

    AppSettings& m_settings;
    HWND m_owner = nullptr;

    HFONT m_normalFont = nullptr;
    HFONT m_smallFont = nullptr;
    HFONT m_boldFont = nullptr;
    HFONT m_italicFont = nullptr;
    HFONT m_underlineFont = nullptr;

    std::unordered_map<std::wstring, IconCacheEntry> m_iconCache;
    std::list<std::wstring> m_iconLru;
    std::vector<int> m_unpinnedShortcutNumbers;
    std::wstring m_highlightQuery;
    SearchMode m_highlightMode = SearchMode::Exact;
    std::optional<std::wregex> m_highlightRegex;
    bool m_highlightPatternReady = false;
};
