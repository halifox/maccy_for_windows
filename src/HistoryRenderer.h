#pragma once

#include "PlatformConfig.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "Database.h"
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

    // Drawing operations
    void DrawHistoryItem(DRAWITEMSTRUCT* draw,
                         const std::vector<ClipboardItem>& items,
                         int activeIndex,
                         int activeFooter,
                         std::wstring_view searchQuery);
    void DrawMenuButton(DRAWITEMSTRUCT* draw,
                        int activeFooter,
                        const std::array<HWND, 4>& footerButtons);
    void OnPaint(HDC dc, const RECT& client,
                 int pinSeparatorY,
                 int footerSeparatorY,
                 bool showSearch,
                 const RECT& searchRect,
                 const RECT& titleRect);

    // Font access
    HFONT GetNormalFont() const { return m_normalFont; }
    HFONT GetSmallFont() const { return m_smallFont; }
    HFONT GetBoldFont() const { return m_boldFont; }

    // Text utilities
    std::wstring DisplayText(const ClipboardItem& item) const;
    static std::wstring MakeTitle(std::wstring value, bool show_special_symbols);
    static std::wstring PreviewText(std::wstring_view text);

private:
    struct HistoryItemLayout {
        RECT background{};
        RECT icon{};
        RECT attachment{};
        RECT content{};
        RECT shortcut{};
    };

    // Layout
    HistoryItemLayout LayoutHistoryItem(const RECT& row, const ClipboardItem& item) const;

    // Drawing helpers
    void DrawTextWithHighlights(HDC dc, RECT rect, std::wstring_view text,
                                bool selected, std::wstring_view searchQuery);
    std::vector<std::pair<size_t, size_t>> HighlightRanges(std::wstring_view text,
                                                            std::wstring_view searchQuery) const;
    HFONT FontForHighlight(HighlightMatch match) const;

    // Icon management
    HICON IconForApplication(std::wstring_view application);

    AppSettings& m_settings;
    HWND m_owner = nullptr;

    HFONT m_normalFont = nullptr;
    HFONT m_smallFont = nullptr;
    HFONT m_boldFont = nullptr;
    HFONT m_italicFont = nullptr;
    HFONT m_underlineFont = nullptr;

    std::unordered_map<std::wstring, HICON> m_iconCache;
};
