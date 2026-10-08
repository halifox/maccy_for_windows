#include "HistoryRenderer.h"
#include "UiFont.h"
#include "ClipboardRules.h"
#include "GdiScope.h"
#include "Localization.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <optional>
#include <regex>

#include <shellapi.h>
#include <shlobj.h>

#include "resource.h"

namespace {

constexpr int kHistoryItemInset = 2;
constexpr int kHistoryItemLeftPadding = 10;
constexpr int kHistoryItemRightPadding = 10;
constexpr int kHistoryItemSlot = 16;
constexpr int kHistoryItemSlotGap = 6;
constexpr int kHistoryShortcutWidth = 74;
constexpr int kHistoryItemRadius = 4;

std::wstring ReadWindowText(CWindow window) {
    if (window.m_hWnd == nullptr) {
        return {};
    }
    const int length = window.GetWindowTextLength();
    if (length <= 0) {
        return {};
    }
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = window.GetWindowText(text.data(), length + 1);
    text.resize(static_cast<size_t>(std::max(copied, 0)));
    return text;
}

int HexDigit(wchar_t character) {
    if (character >= L'0' && character <= L'9') {
        return character - L'0';
    }
    if (character >= L'a' && character <= L'f') {
        return character - L'a' + 10;
    }
    if (character >= L'A' && character <= L'F') {
        return character - L'A' + 10;
    }
    return -1;
}

std::optional<COLORREF> ParseHexColor(std::wstring_view text) {
    const size_t digit_start = !text.empty() && text.front() == L'#' ? 1 : 0;
    const size_t digit_count = text.size() - digit_start;
    if (digit_count != 3 && digit_count != 6) {
        return std::nullopt;
    }

    unsigned int value = 0;
    for (size_t index = digit_start; index < text.size(); ++index) {
        const int digit = HexDigit(text[index]);
        if (digit < 0) {
            return std::nullopt;
        }
        value = (value << 4) | static_cast<unsigned int>(digit);
    }

    if (digit_count == 3) {
        const unsigned int red = ((value >> 8) & 0x0F) * 0x11;
        const unsigned int green = ((value >> 4) & 0x0F) * 0x11;
        const unsigned int blue = (value & 0x0F) * 0x11;
        return RGB(red, green, blue);
    }

    return RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

} // namespace

HistoryRenderer::HistoryRenderer(AppSettings& settings)
    : m_settings(settings) {}

HistoryRenderer::~HistoryRenderer() {
    Shutdown();
}

bool HistoryRenderer::Initialize(CWindow owner) {
    return UpdateFonts(UiFont::DpiForWindow(owner.m_hWnd));
}

bool HistoryRenderer::UpdateFonts(UINT dpi) {
    CFont normalFont;
    CFont smallFont;
    CFont boldFont;
    CFont italicFont;
    CFont underlineFont;
    normalFont.Attach(UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize));
    smallFont.Attach(UiFont::CreateSegoeUi(dpi, UiFont::kSmallPointSize));
    boldFont.Attach(UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize, FW_BOLD));
    italicFont.Attach(UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize, FW_NORMAL, TRUE));
    underlineFont.Attach(UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize, FW_NORMAL, FALSE, TRUE));
    if (normalFont.IsNull() || smallFont.IsNull() || boldFont.IsNull() ||
        italicFont.IsNull() || underlineFont.IsNull()) {
        return false;
    }

    m_normalFont.DeleteObject();
    m_smallFont.DeleteObject();
    m_boldFont.DeleteObject();
    m_italicFont.DeleteObject();
    m_underlineFont.DeleteObject();
    m_normalFont.Attach(normalFont.Detach());
    m_smallFont.Attach(smallFont.Detach());
    m_boldFont.Attach(boldFont.Detach());
    m_italicFont.Attach(italicFont.Detach());
    m_underlineFont.Attach(underlineFont.Detach());
    return true;
}

void HistoryRenderer::Shutdown() {
    m_normalFont.DeleteObject();
    m_smallFont.DeleteObject();
    m_boldFont.DeleteObject();
    m_italicFont.DeleteObject();
    m_underlineFont.DeleteObject();

    m_iconCache.clear();
    m_iconLru.clear();
    m_imageThumbnails.clear();
    m_unpinnedShortcutNumbers.clear();
    m_highlightQuery.clear();
    m_highlightRegex.reset();
    m_highlightPatternReady = false;
}

void HistoryRenderer::PrepareHistory(const std::vector<ClipboardItem> &items) {
    m_unpinnedShortcutNumbers.assign(items.size(), 0);
    int number = 0;
    for (size_t index = 0; index < items.size(); ++index) {
        if (!items[index].pinned) {
            ++number;
            if (number <= 9) {
                m_unpinnedShortcutNumbers[index] = number;
            }
        }
    }
}

void HistoryRenderer::SetImageThumbnails(std::unordered_map<sqlite3_int64, PreviewBitmap> thumbnails) {
    m_imageThumbnails = std::move(thumbnails);
}

void HistoryRenderer::SetImageThumbnail(sqlite3_int64 id, PreviewBitmap thumbnail) {
    m_imageThumbnails.insert_or_assign(id, std::move(thumbnail));
}

const PreviewBitmap* HistoryRenderer::ImageThumbnail(sqlite3_int64 id) const noexcept {
    const auto it = m_imageThumbnails.find(id);
    return it == m_imageThumbnails.end() ? nullptr : &it->second;
}

void HistoryRenderer::ClearImageThumbnails() noexcept {
    m_imageThumbnails.clear();
}

std::wstring HistoryRenderer::DisplayText(const ClipboardItem& item) const {
    const std::wstring image_placeholder = Localization::Text(IDS_CONTENT_IMAGE);
    if (!item.title.empty() && item.title != image_placeholder) {
        return item.title;
    }
    if (!item.preview.empty() && item.preview != image_placeholder) {
        return ClipboardRules::PreviewText(item.preview);
    }
    if (item.has_image) {
        return {};
    }
    if (item.has_files) {
        return Localization::Text(IDS_CONTENT_FILE);
    }
    return Localization::Text(IDS_CONTENT_CLIPBOARD_ITEM);
}

HistoryRenderer::HistoryItemLayout HistoryRenderer::LayoutHistoryItem(
    const RECT& row,
    const ClipboardItem& item,
    bool has_color_swatch
) const {
    HistoryItemLayout layout{};
    layout.background = row;
    layout.background.left += kHistoryItemInset;
    layout.background.right -= kHistoryItemInset;
    layout.shortcut = row;
    layout.shortcut.left = std::max(row.left, row.right - kHistoryItemRightPadding - kHistoryShortcutWidth);
    layout.shortcut.right = row.right - kHistoryItemRightPadding;
    int x = row.left + kHistoryItemLeftPadding;
    const int y = row.top + std::max<LONG>(0, ((row.bottom - row.top) - kHistoryItemSlot) / 2);
    if (m_settings.show_application_icons && !item.application.empty()) {
        layout.icon = {x, y, x + kHistoryItemSlot, y + kHistoryItemSlot};
        x += kHistoryItemSlot + kHistoryItemSlotGap;
    }
    if (item.has_image) {
        if (const auto *thumbnail = ImageThumbnail(item.id); thumbnail != nullptr) {
            layout.attachment = {x, row.top + std::max<LONG>(0, ((row.bottom - row.top) - thumbnail->height) / 2),
                                 x + thumbnail->width, row.top + std::max<LONG>(0, ((row.bottom - row.top) - thumbnail->height) / 2) + thumbnail->height};
            x += thumbnail->width + kHistoryItemSlotGap;
        } else {
            layout.attachment = {x, y, x + kHistoryItemSlot, y + kHistoryItemSlot};
            x += kHistoryItemSlot + kHistoryItemSlotGap;
        }
    } else if (item.has_files) {
        layout.attachment = {x, y, x + kHistoryItemSlot, y + kHistoryItemSlot};
        x += kHistoryItemSlot + kHistoryItemSlotGap;
    }
    if (has_color_swatch) {
        layout.swatch = {x, y, x + kHistoryItemSlot, y + kHistoryItemSlot};
        x += kHistoryItemSlot + kHistoryItemSlotGap;
    }
    layout.content = row;
    layout.content.left = x;
    layout.content.right = std::max<LONG>(x, layout.shortcut.left - kHistoryItemSlotGap);
    return layout;
}

HFONT HistoryRenderer::FontForHighlight(HighlightMatch match) const {
    switch (match) {
    case HighlightMatch::Bold:
        return m_boldFont;
    case HighlightMatch::Italic:
        return m_italicFont;
    case HighlightMatch::Underline:
        return m_underlineFont;
    case HighlightMatch::Color:
    default:
        return m_normalFont;
    }
}

void HistoryRenderer::PrepareHighlightPattern(std::wstring_view searchQuery) {
    if (m_highlightPatternReady && m_highlightQuery == searchQuery &&
        m_highlightMode == m_settings.search_mode) {
        return;
    }

    m_highlightQuery.assign(searchQuery);
    m_highlightMode = m_settings.search_mode;
    m_highlightRegex.reset();
    if (!searchQuery.empty() &&
        (m_highlightMode == SearchMode::Regexp || m_highlightMode == SearchMode::Mixed)) {
        try {
            m_highlightRegex.emplace(std::wstring(searchQuery));
        } catch (const std::regex_error&) {
            m_highlightRegex.reset();
        }
    }
    m_highlightPatternReady = true;
}

std::vector<std::pair<size_t, size_t>> HistoryRenderer::HighlightRanges(
    std::wstring_view text, std::wstring_view searchQuery) {
    PrepareHighlightPattern(searchQuery);
    std::vector<std::pair<size_t, size_t>> ranges;
    if (searchQuery.empty()) {
        return ranges;
    }
    const auto add_exact = [&]() {
        const std::wstring lower_text = ClipboardRules::Lower(text);
        const std::wstring lower_query = ClipboardRules::Lower(searchQuery);
        if (lower_query.empty()) {
            return;
        }
        size_t position = 0;
        while ((position = lower_text.find(lower_query, position)) != std::wstring::npos) {
            ranges.emplace_back(position, position + lower_query.size());
            position += lower_query.size();
        }
    };
    const auto add_regex = [&]() {
        if (!m_highlightRegex) {
            return;
        }
        const std::wstring value(text);
        for (std::wsregex_iterator it(value.begin(), value.end(), *m_highlightRegex), end;
             it != end; ++it) {
            const auto match = *it;
            ranges.emplace_back(
                static_cast<size_t>(match.position()),
                static_cast<size_t>(match.position() + match.length())
            );
        }
    };
    const auto add_fuzzy = [&]() {
        const std::wstring lower_text = ClipboardRules::Lower(text);
        const std::wstring lower_query = ClipboardRules::Lower(searchQuery);
        size_t text_position = 0;
        size_t start = std::wstring::npos;
        size_t last = std::wstring::npos;
        for (const wchar_t expected : lower_query) {
            const size_t found = lower_text.find(expected, text_position);
            if (found == std::wstring::npos) {
                ranges.clear();
                return;
            }
            if (start == std::wstring::npos) {
                start = found;
            }
            last = found;
            ranges.emplace_back(found, found + 1);
            text_position = found + 1;
        }
    };

    switch (m_settings.search_mode) {
    case SearchMode::Regexp:
        add_regex();
        break;
    case SearchMode::Fuzzy:
        add_fuzzy();
        break;
    case SearchMode::Mixed:
        add_exact();
        if (ranges.empty()) {
            add_regex();
        }
        if (ranges.empty()) {
            add_fuzzy();
        }
        break;
    case SearchMode::Exact:
    default:
        add_exact();
        break;
    }
    return ranges;
}

void HistoryRenderer::DrawTextWithHighlights(HDC dc, RECT rect, std::wstring_view text,
                                            bool selected, std::wstring_view searchQuery) {
    ScopedDcState dc_state(dc);
    IntersectClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
    const auto originalRanges = HighlightRanges(text, searchQuery);
    const auto measure = [&](std::wstring_view value) {
        SIZE size{};
        // Use the bold font for fitting so highlighted text also fits.
        ScopedGdiObjectSelection font_selection(
            dc,
            m_boldFont != nullptr ? m_boldFont : GetStockObject(DEFAULT_GUI_FONT)
        );
        GetTextExtentPoint32W(dc, value.data(), static_cast<int>(value.size()), &size);
        return size.cx;
    };
    std::wstring display(text);
    size_t prefix = text.size(), suffix = 0;
    if (measure(text) > rect.right - rect.left) {
        size_t low = 0, high = text.size();
        while (low < high) {
            const size_t length = (low + high + 1) / 2;
            const auto candidate = std::wstring(text.substr(0, (length + 1) / 2)) + L"…" + std::wstring(text.substr(text.size() - length / 2));
            if (measure(candidate) <= rect.right - rect.left) low = length; else high = length - 1;
        }
        prefix = (low + 1) / 2; suffix = low / 2;
        if (prefix && text[prefix - 1] >= 0xd800 && text[prefix - 1] <= 0xdbff) --prefix;
        if (suffix && text[text.size() - suffix] >= 0xdc00 && text[text.size() - suffix] <= 0xdfff) --suffix;
        display = std::wstring(text.substr(0, prefix)) + L"…" + std::wstring(text.substr(text.size() - suffix));
    }
    std::vector<std::pair<size_t, size_t>> ranges;
    for (const auto& [start, end] : originalRanges) {
        if (start < prefix) ranges.emplace_back(start, std::min(end, prefix));
        if (suffix && end > text.size() - suffix) {
            const size_t from = std::max(start, text.size() - suffix);
            ranges.emplace_back(prefix + 1 + from - (text.size() - suffix), prefix + 1 + end - (text.size() - suffix));
        }
    }
    text = display;
    ScopedGdiObjectSelection normal_font_selection(
        dc,
        m_normalFont != nullptr ? m_normalFont : GetStockObject(DEFAULT_GUI_FONT)
    );
    SetTextColor(dc, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
    std::vector<std::pair<size_t, size_t>> segments;
    size_t position = 0;
    for (const auto& [start, end] : ranges) {
        if (start > position) {
            segments.emplace_back(position, start);
        }
        segments.emplace_back(start, std::max(start, end));
        position = std::max(position, end);
    }
    if (position < text.size()) {
        segments.emplace_back(position, text.size());
    }
    if (segments.empty()) {
        segments.emplace_back(0, text.size());
    }

    const int old_bk_mode = SetBkMode(dc, TRANSPARENT);
    const COLORREF old_text = GetTextColor(dc);
    int x = rect.left;
    for (const auto& [start, end] : segments) {
        if (end <= start) {
            continue;
        }
        const bool highlighted = std::any_of(
            ranges.begin(),
            ranges.end(),
            [start, end](const auto& range) { return start >= range.first && end <= range.second; }
        );
        const std::wstring part(text.substr(start, end - start));
        SIZE size{};
        HFONT font = highlighted
            ? FontForHighlight(m_settings.highlight_match)
            : static_cast<HFONT>(m_normalFont);
        ScopedGdiObjectSelection font_selection(
            dc,
            font != nullptr ? font : GetStockObject(DEFAULT_GUI_FONT)
        );
        if (highlighted && m_settings.highlight_match == HighlightMatch::Color) {
            SetTextColor(dc, RGB(0, 70, 160));
            SetBkMode(dc, OPAQUE);
            SetBkColor(dc, RGB(255, 239, 160));
        } else {
            SetTextColor(dc, selected ? GetSysColor(COLOR_HIGHLIGHTTEXT) : old_text);
            SetBkMode(dc, TRANSPARENT);
        }
        GetTextExtentPoint32W(dc, part.c_str(), static_cast<int>(part.size()), &size);
        TextOutW(dc, x, rect.top, part.c_str(), static_cast<int>(part.size()));
        x += size.cx;
        if (x >= rect.right) {
            break;
        }
    }
    SetTextColor(dc, old_text);
    SetBkMode(dc, old_bk_mode);
}

HICON HistoryRenderer::IconForApplication(std::wstring_view application) {
    if (application.empty()) {
        return nullptr;
    }
    const std::wstring key = ClipboardRules::NormalizePath(std::wstring(application));
    if (const auto found = m_iconCache.find(key); found != m_iconCache.end()) {
        m_iconLru.splice(m_iconLru.begin(), m_iconLru, found->second.lru);
        return found->second.icon.Get();
    }
    SHFILEINFOW info{};
    if (SHGetFileInfoW(
        application.data(),
        0,
        &info,
        sizeof(info),
        SHGFI_ICON | SHGFI_SMALLICON
    ) == 0) {
        info.hIcon = nullptr;
    }
    UniqueIcon icon(info.hIcon);
    if (m_iconCache.size() >= 32) {
        const std::wstring evicted_key = m_iconLru.back();
        m_iconLru.pop_back();
        const auto evicted = m_iconCache.find(evicted_key);
        if (evicted != m_iconCache.end()) {
            m_iconCache.erase(evicted);
        }
    }
    m_iconLru.push_front(key);
    const HICON result = icon.Get();
    m_iconCache.emplace(key, IconCacheEntry{std::move(icon), m_iconLru.begin()});
    return result;
}

void HistoryRenderer::DrawHistoryItem(DRAWITEMSTRUCT* draw,
                                     const std::vector<ClipboardItem>& items,
                                     int activeIndex,
                                     int activeFooter,
                                     std::wstring_view searchQuery) {
    if (draw == nullptr || draw->itemID == static_cast<UINT>(-1) ||
        static_cast<size_t>(draw->itemData) >= items.size()) {
        return;
    }
    if (m_unpinnedShortcutNumbers.size() != items.size()) {
        PrepareHistory(items);
    }
    const int index = static_cast<int>(draw->itemData);
    const ClipboardItem& item = items[index];
    const bool selected = activeFooter < 0 && index == activeIndex;
    const std::wstring text = DisplayText(item);
    std::optional<COLORREF> swatch_color;
    if (m_settings.show_hex_color_swatch) {
        swatch_color = ParseHexColor(text);
    }
    const HistoryItemLayout layout = LayoutHistoryItem(draw->rcItem, item, swatch_color.has_value());
    ScopedDcState dc_state(draw->hDC);
    FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_WINDOW));
    if (selected) {
        {
            ScopedGdiObjectSelection null_pen(draw->hDC, GetStockObject(NULL_PEN));
            ScopedGdiObjectSelection highlight_brush(
                draw->hDC,
                GetSysColorBrush(COLOR_HIGHLIGHT)
            );
            RECT selected_rect = layout.background;
            const bool previous_selected = index > 0 && index - 1 == activeIndex;
            const bool next_selected = index + 1 < static_cast<int>(items.size()) && index + 1 == activeIndex;
            if (previous_selected) selected_rect.top = draw->rcItem.top;
            if (next_selected) selected_rect.bottom = draw->rcItem.bottom;
            RoundRect(draw->hDC, selected_rect.left, selected_rect.top, selected_rect.right, selected_rect.bottom,
                kHistoryItemRadius, kHistoryItemRadius);
            if (previous_selected || next_selected) {
                RECT join = selected_rect;
                join.left += kHistoryItemRadius / 2;
                join.right -= kHistoryItemRadius / 2;
                FillRect(draw->hDC, &join, GetSysColorBrush(COLOR_HIGHLIGHT));
            }
        }
    }
    ScopedGdiObjectSelection normal_font_selection(
        draw->hDC,
        m_normalFont != nullptr ? m_normalFont : GetStockObject(DEFAULT_GUI_FONT)
    );

    TEXTMETRICW metrics{};
    const int text_height = GetTextMetricsW(draw->hDC, &metrics) != FALSE && metrics.tmHeight > 0
        ? metrics.tmHeight
        : 16;
    const int row_height = std::max(1L, draw->rcItem.bottom - draw->rcItem.top);
    const int text_top = draw->rcItem.top + std::max(0, (row_height - text_height) / 2);
    RECT text_rect = layout.content;
    text_rect.top = text_top;
    text_rect.bottom = text_top + text_height;

    if (!IsRectEmpty(&layout.icon)) {
        if (const HICON icon = IconForApplication(item.application)) {
            DrawIconEx(draw->hDC, layout.icon.left, layout.icon.top, icon, 16, 16, 0, nullptr, DI_NORMAL);
        }
    }
    if (!IsRectEmpty(&layout.attachment)) {
        const auto thumbnail = m_imageThumbnails.find(item.id);
        if (item.has_image && thumbnail != m_imageThumbnails.end() && thumbnail->second.handle != nullptr) {
            const int width = thumbnail->second.width;
            const int height = thumbnail->second.height;
            const int x = layout.attachment.left;
            const int y = draw->rcItem.top + std::max(0, (row_height - height) / 2);
            CDC memory;
            if (memory.CreateCompatibleDC(draw->hDC)) {
                ScopedGdiObjectSelection bitmap(memory.m_hDC, thumbnail->second.handle);
                if (bitmap.IsSelected()) {
                    ScopedDcState thumbnail_dc_state(draw->hDC);
                    ::SetStretchBltMode(draw->hDC, HALFTONE);
                    ::SetBrushOrgEx(draw->hDC, 0, 0, nullptr);
                    ::StretchBlt(draw->hDC, x, y, width, height, memory.m_hDC, 0, 0, width, height, SRCCOPY);
                }
            }
        } else {
            const COLORREF marker = item.has_image ? RGB(90, 105, 120) : RGB(170, 125, 35);
            CPen marker_pen;
            if (marker_pen.CreatePen(PS_SOLID, 1, marker)) {
                ScopedGdiObjectSelection pen_selection(draw->hDC, marker_pen);
                ScopedGdiObjectSelection brush_selection(draw->hDC, GetStockObject(NULL_BRUSH));
                Rectangle(draw->hDC, layout.attachment.left + 1, layout.attachment.top + 2,
                    layout.attachment.right - 1, layout.attachment.bottom - 2);
            }
        }
    }

    if (!IsRectEmpty(&layout.swatch) && swatch_color.has_value()) {
        CBrush brush;
        if (brush.CreateSolidBrush(*swatch_color)) {
            FillRect(draw->hDC, &layout.swatch, brush);
        }
        FrameRect(draw->hDC, &layout.swatch, GetSysColorBrush(COLOR_GRAYTEXT));
    }

    std::wstring shortcut;
    if (item.pinned) shortcut = L"Ctrl+" + item.pin;
    else if (m_unpinnedShortcutNumbers[static_cast<size_t>(index)] > 0) {
        shortcut = L"Ctrl+" + std::to_wstring(
            m_unpinnedShortcutNumbers[static_cast<size_t>(index)]
        );
    }
    RECT keyRect = layout.shortcut;
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT));
    DrawTextW(draw->hDC, shortcut.c_str(), -1, &keyRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    text_rect.right = layout.content.right;
    DrawTextWithHighlights(draw->hDC, text_rect, text, selected, searchQuery);
}

void HistoryRenderer::DrawMenuButton(DRAWITEMSTRUCT* draw,
                                    int activeFooter,
                                    const std::array<CButton, AppConstants::UI::kFooterButtonCount>& footerButtons) {
    auto it = std::find_if(footerButtons.begin(), footerButtons.end(), [draw](const CButton& button) {
        return button.m_hWnd == draw->hwndItem;
    });
    const int index = it == footerButtons.end() ? -1 : static_cast<int>(it - footerButtons.begin());
    const bool selected = index >= 0 && index == activeFooter;
    ScopedDcState dc_state(draw->hDC);
    FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_WINDOW));
    if (selected || (draw->itemState & ODS_SELECTED)) {
        ScopedGdiObjectSelection null_pen(draw->hDC, GetStockObject(NULL_PEN));
        ScopedGdiObjectSelection background_brush(
            draw->hDC,
            GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_BTNFACE)
        );
        RECT selected_rect = draw->rcItem;
        selected_rect.left += kHistoryItemInset;
        selected_rect.right -= kHistoryItemInset;
        RoundRect(draw->hDC, selected_rect.left, selected_rect.top,
            selected_rect.right, selected_rect.bottom, kHistoryItemRadius, kHistoryItemRadius);
    }
    ScopedGdiObjectSelection normal_font_selection(
        draw->hDC,
        m_normalFont != nullptr ? m_normalFont : GetStockObject(DEFAULT_GUI_FONT)
    );
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
    RECT rect = draw->rcItem;
    rect.left += kHistoryItemLeftPadding;
    rect.right -= kHistoryItemRightPadding;
    if (draw->CtlID == IDC_HISTORY_PREVIEW) {
        const int width = draw->rcItem.right - draw->rcItem.left;
        const int height = draw->rcItem.bottom - draw->rcItem.top;
        const UINT dpi = UiFont::DpiForWindow(draw->hwndItem);
        const int iconWidth = std::min(width - MulDiv(6, static_cast<int>(dpi), 96),
                                       MulDiv(18, static_cast<int>(dpi), 96));
        const int iconHeight = std::min(height - MulDiv(6, static_cast<int>(dpi), 96),
                                        MulDiv(14, static_cast<int>(dpi), 96));
        const int left = draw->rcItem.left + (width - iconWidth) / 2;
        const int top = draw->rcItem.top + (height - iconHeight) / 2;
        CPen pen;
        if (pen.CreatePen(PS_SOLID, 1, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT))) {
            ScopedGdiObjectSelection pen_selection(draw->hDC, pen);
            ScopedGdiObjectSelection brush_selection(draw->hDC, GetStockObject(NULL_BRUSH));
            Rectangle(draw->hDC, left, top, left + iconWidth, top + iconHeight);
            const int divider = left + iconWidth / 2;
            MoveToEx(draw->hDC, divider, top, nullptr);
            LineTo(draw->hDC, divider, top + iconHeight);
        }
        return;
    }
    const auto title = ReadWindowText(CWindow(draw->hwndItem));
    RECT key_rect{};
    const wchar_t* key = nullptr;
    if (index >= 0) {
        const wchar_t* keys[] = {
            (GetKeyState(VK_SHIFT) & 0x8000) ? L"Ctrl+Alt+Shift+Backspace" : L"Ctrl+Alt+Backspace",
            L"Ctrl+,",
            L"",
            L"Ctrl+Q"
        };
        key = keys[index];
        if (*key != L'\0') {
            SIZE key_size{};
            GetTextExtentPoint32W(draw->hDC, key, static_cast<int>(wcslen(key)), &key_size);
            key_rect = draw->rcItem;
            key_rect.right -= kHistoryItemRightPadding;
            key_rect.left = std::max<LONG>(rect.left, key_rect.right - key_size.cx);
            rect.right = std::max<LONG>(rect.left, key_rect.left - kHistoryItemSlotGap);
        }
    }
    DrawTextW(draw->hDC, title.c_str(), -1, &rect,
        DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_LEFT);
    if (key != nullptr && *key != L'\0') {
        SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT));
        DrawTextW(draw->hDC, key, -1, &key_rect,
            DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

void HistoryRenderer::OnPaint(HDC dc, const RECT& client,
                             int pinSeparatorY,
                             int footerSeparatorY,
                             const SearchHeaderLayout::Geometry& header,
                             bool showSearchClear) {
    ScopedDcState dc_state(dc);
    FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));
    CPen separator;
    if (separator.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT))) {
        ScopedGdiObjectSelection pen_selection(dc, separator);
        for (int y : {pinSeparatorY, footerSeparatorY}) if (y >= 0) {
            MoveToEx(dc, 16, y, nullptr); LineTo(dc, client.right - 16, y);
        }
    }
    if (header.showSearch) {
        {
            ScopedGdiObjectSelection null_pen(dc, GetStockObject(NULL_PEN));
            ScopedGdiObjectSelection background_brush(dc, GetSysColorBrush(COLOR_BTNFACE));
            RoundRect(dc, header.search.left, header.search.top,
                      header.search.right, header.search.bottom,
                      header.metrics.cornerRadius, header.metrics.cornerRadius);
        }
        CPen iconPen;
        if (iconPen.CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT))) {
            {
                ScopedGdiObjectSelection pen_selection(dc, iconPen);
                ScopedGdiObjectSelection brush_selection(dc, GetStockObject(NULL_BRUSH));
                const SearchHeaderLayout::IconGeometry icon = SearchHeaderLayout::IconFor(header);
                Ellipse(dc, icon.left, icon.top,
                        icon.left + header.metrics.iconLensSize,
                        icon.top + header.metrics.iconLensSize);
                MoveToEx(dc, icon.left + header.metrics.iconHandleStart,
                         icon.top + header.metrics.iconHandleStart, nullptr);
                LineTo(dc, icon.left + header.metrics.iconHandleEnd,
                       icon.top + header.metrics.iconHandleEnd);
                if (showSearchClear && !IsRectEmpty(&header.searchClear)) {
                    const int centerX = (header.searchClear.left + header.searchClear.right) / 2;
                    const int centerY = (header.searchClear.top + header.searchClear.bottom) / 2;
                    const int halfSize = std::max(2, MulDiv(3, header.metrics.clearWidth,
                                                             SearchHeaderLayout::kClearWidth));
                    MoveToEx(dc, centerX - halfSize, centerY - halfSize, nullptr);
                    LineTo(dc, centerX + halfSize + 1, centerY + halfSize + 1);
                    MoveToEx(dc, centerX + halfSize, centerY - halfSize, nullptr);
                    LineTo(dc, centerX - halfSize - 1, centerY + halfSize + 1);
                }
            }
        }
    }
    if (m_settings.show_title && header.showTitle && !IsRectEmpty(&header.title)) {
        const HFONT title_font = m_smallFont.IsNull()
            ? static_cast<HFONT>(m_normalFont)
            : static_cast<HFONT>(m_smallFont);
        ScopedGdiObjectSelection font_selection(dc, title_font);
        const int previous_color = SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
        const int previous_mode = SetBkMode(dc, TRANSPARENT);
        RECT title = header.title;
        DrawTextW(dc, L"maccy", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SetBkMode(dc, previous_mode);
        SetTextColor(dc, previous_color);
    }
}
