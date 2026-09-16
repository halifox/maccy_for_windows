#include "HistoryRenderer.h"
#include "UiFont.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <optional>
#include <regex>

#include <shellapi.h>
#include <shlobj.h>

#include "resource.h"

namespace {

constexpr int kHistoryItemHeight = 22;
constexpr int kHistoryItemInset = 2;
constexpr int kHistoryItemRadius = 4;
constexpr int kHistoryItemLeftPadding = 10;
constexpr int kHistoryItemRightPadding = 10;
constexpr int kHistoryItemSlot = 16;
constexpr int kHistoryItemSlotGap = 6;
constexpr int kHistoryShortcutWidth = 74;

std::wstring Lower(std::wstring_view value) {
    std::wstring result;
    result.reserve(value.size());
    for (const wchar_t character : value) {
        result.push_back(static_cast<wchar_t>(std::towlower(character)));
    }
    return result;
}

std::wstring Trim(std::wstring value) {
    const auto is_space = [](wchar_t character) { return std::iswspace(character) != 0; };
    const auto first = std::find_if_not(value.begin(), value.end(), is_space);
    const auto last = std::find_if_not(value.rbegin(), value.rend(), is_space).base();
    if (first >= last) {
        return {};
    }
    return std::wstring(first, last);
}

std::wstring NormalizePath(std::wstring value) {
    value = Lower(Trim(std::move(value)));
    while (!value.empty() && (value.back() == L'\\' || value.back() == L'/')) {
        value.pop_back();
    }
    return value;
}

std::wstring ReadWindowText(HWND window) {
    if (window == nullptr) {
        return {};
    }
    const int length = GetWindowTextLengthW(window);
    if (length <= 0) {
        return {};
    }
    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    const int copied = GetWindowTextW(window, text.data(), length + 1);
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

bool HistoryRenderer::Initialize(HWND owner) {
    m_owner = owner;
    return UpdateFonts(UiFont::DpiForWindow(owner));
}

bool HistoryRenderer::UpdateFonts(UINT dpi) {
    HFONT normalFont = UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize);
    HFONT smallFont = UiFont::CreateSegoeUi(dpi, UiFont::kSmallPointSize);
    HFONT boldFont = UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize, FW_BOLD);
    HFONT italicFont = UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize, FW_NORMAL, TRUE);
    HFONT underlineFont = UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize, FW_NORMAL, FALSE, TRUE);
    if (normalFont == nullptr || smallFont == nullptr || boldFont == nullptr ||
        italicFont == nullptr || underlineFont == nullptr) {
        for (HFONT font : {normalFont, smallFont, boldFont, italicFont, underlineFont}) {
            if (font != nullptr) DeleteObject(font);
        }
        return false;
    }

    for (HFONT font : {m_normalFont, m_smallFont, m_boldFont, m_italicFont, m_underlineFont}) {
        if (font != nullptr) DeleteObject(font);
    }
    m_normalFont = normalFont;
    m_smallFont = smallFont;
    m_boldFont = boldFont;
    m_italicFont = italicFont;
    m_underlineFont = underlineFont;
    return true;
}

void HistoryRenderer::Shutdown() {
    for (HFONT font : {m_normalFont, m_smallFont, m_boldFont, m_italicFont, m_underlineFont}) {
        if (font != nullptr) {
            DeleteObject(font);
        }
    }
    m_normalFont = nullptr;
    m_smallFont = nullptr;
    m_boldFont = nullptr;
    m_italicFont = nullptr;
    m_underlineFont = nullptr;

    for (auto& [path, icon] : m_iconCache) {
        (void)path;
        if (icon != nullptr) {
            DestroyIcon(icon);
        }
    }
    m_iconCache.clear();
}

std::wstring HistoryRenderer::MakeTitle(std::wstring value, bool show_special_symbols) {
    value.resize(std::min<size_t>(value.size(), 1000));
    if (!show_special_symbols) {
        return Trim(std::move(value));
    }

    size_t leading = 0;
    while (leading < value.size() && value[leading] == L' ') {
        value[leading++] = L'\x00b7';
    }
    size_t trailing = value.size();
    while (trailing > 0 && value[trailing - 1] == L' ') {
        value[--trailing] = L'\x00b7';
    }

    std::wstring result;
    result.reserve(value.size() + 8);
    for (const wchar_t character : value) {
        switch (character) {
        case L'\r':
            break;
        case L'\n':
            result += L'\x23ce';
            break;
        case L'\t':
            result += L'\x21e5';
            break;
        default:
            result += character;
            break;
        }
    }
    return result;
}

std::wstring HistoryRenderer::PreviewText(std::wstring_view text) {
    constexpr size_t kPreviewCharacters = 180;
    std::wstring preview;
    preview.reserve(std::min(text.size(), kPreviewCharacters + 3));
    for (const wchar_t character : text) {
        if (preview.size() >= kPreviewCharacters) {
            preview += L"...";
            break;
        }
        preview += (character < L' ' && character != L'\t') ? L' ' : character;
    }
    return preview;
}

std::wstring HistoryRenderer::DisplayText(const ClipboardItem& item) const {
    if (!item.title.empty()) {
        return item.title;
    }
    if (!item.preview.empty()) {
        return PreviewText(item.preview);
    }
    if (item.has_image) {
        return L"[图片]";
    }
    if (item.has_files) {
        return L"[文件]";
    }
    return L"[剪贴板项目]";
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
    if (item.has_image || item.has_files) {
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

std::vector<std::pair<size_t, size_t>> HistoryRenderer::HighlightRanges(
    std::wstring_view text, std::wstring_view searchQuery) const {
    std::vector<std::pair<size_t, size_t>> ranges;
    if (searchQuery.empty()) {
        return ranges;
    }
    const auto add_exact = [&]() {
        const std::wstring lower_text = Lower(text);
        const std::wstring lower_query = Lower(searchQuery);
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
        try {
            const std::wregex expression{std::wstring(searchQuery)};
            const std::wstring value(text);
            for (std::wsregex_iterator it(value.begin(), value.end(), expression), end; it != end; ++it) {
                const auto match = *it;
                ranges.emplace_back(
                    static_cast<size_t>(match.position()),
                    static_cast<size_t>(match.position() + match.length())
                );
            }
        } catch (const std::regex_error&) {
        }
    };
    const auto add_fuzzy = [&]() {
        const std::wstring lower_text = Lower(text);
        const std::wstring lower_query = Lower(searchQuery);
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
    const int saved = SaveDC(dc);
    IntersectClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
    const auto originalRanges = HighlightRanges(text, searchQuery);
    const auto measure = [&](std::wstring_view value) {
        SIZE size{};
        // Use the bold font for fitting so highlighted text also fits.
        SelectObject(dc, m_boldFont);
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
    SelectObject(dc, m_normalFont);
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
        HFONT font = highlighted ? FontForHighlight(m_settings.highlight_match) : m_normalFont;
        HGDIOBJ old_font = SelectObject(dc, font != nullptr ? font : GetStockObject(DEFAULT_GUI_FONT));
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
        SelectObject(dc, old_font);
        if (x >= rect.right) {
            break;
        }
    }
    SetTextColor(dc, old_text);
    SetBkMode(dc, old_bk_mode);
    RestoreDC(dc, saved);
}

HICON HistoryRenderer::IconForApplication(std::wstring_view application) {
    if (application.empty()) {
        return nullptr;
    }
    const std::wstring key = NormalizePath(std::wstring(application));
    if (const auto found = m_iconCache.find(key); found != m_iconCache.end()) {
        return found->second;
    }
    SHFILEINFOW info{};
    if (SHGetFileInfoW(
        application.data(),
        0,
        &info,
        sizeof(info),
        SHGFI_ICON | SHGFI_SMALLICON
    ) == 0) {
        return nullptr;
    }
    if (m_iconCache.size() >= 32) {
        auto first = m_iconCache.begin();
        DestroyIcon(first->second);
        m_iconCache.erase(first);
    }
    m_iconCache.emplace(key, info.hIcon);
    return info.hIcon;
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
    const int index = static_cast<int>(draw->itemData);
    const ClipboardItem& item = items[index];
    const bool selected = activeFooter < 0 && index == activeIndex;
    const std::wstring text = DisplayText(item);
    std::optional<COLORREF> swatch_color;
    if (m_settings.show_hex_color_swatch) {
        swatch_color = ParseHexColor(text);
    }
    const HistoryItemLayout layout = LayoutHistoryItem(draw->rcItem, item, swatch_color.has_value());
    FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_WINDOW));
    if (selected) {
        HGDIOBJ pen = SelectObject(draw->hDC, GetStockObject(NULL_PEN));
        HGDIOBJ brush = SelectObject(draw->hDC, GetSysColorBrush(COLOR_HIGHLIGHT));
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
        SelectObject(draw->hDC, brush); SelectObject(draw->hDC, pen);
    }
    SelectObject(draw->hDC, m_normalFont);

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
        // Keep this marker cheap, but make its meaning visible instead of
        // using an opaque color block. Full payloads remain in SQLite and
        // are still loaded only by the preview/copy path.
        const COLORREF marker = item.has_image ? RGB(90, 105, 120) : RGB(170, 125, 35);
        HPEN marker_pen = CreatePen(PS_SOLID, 1, marker);
        HGDIOBJ old_pen = SelectObject(draw->hDC, marker_pen);
        HGDIOBJ old_brush = SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
        Rectangle(draw->hDC, layout.attachment.left + 1, layout.attachment.top + 2,
            layout.attachment.right - 1, layout.attachment.bottom - 2);
        if (item.has_image) {
            MoveToEx(draw->hDC, layout.attachment.left + 3, layout.attachment.bottom - 4, nullptr);
            LineTo(draw->hDC, layout.attachment.left + 7, layout.attachment.top + 7);
            LineTo(draw->hDC, layout.attachment.left + 10, layout.attachment.bottom - 6);
        } else {
            MoveToEx(draw->hDC, layout.attachment.left + 4, layout.attachment.top + 5, nullptr);
            LineTo(draw->hDC, layout.attachment.right - 4, layout.attachment.top + 5);
        }
        SelectObject(draw->hDC, old_brush);
        SelectObject(draw->hDC, old_pen);
        DeleteObject(marker_pen);
    }

    if (!IsRectEmpty(&layout.swatch) && swatch_color.has_value()) {
        HBRUSH brush = CreateSolidBrush(*swatch_color);
        if (brush != nullptr) {
            FillRect(draw->hDC, &layout.swatch, brush);
            DeleteObject(brush);
        }
        FrameRect(draw->hDC, &layout.swatch, GetSysColorBrush(COLOR_GRAYTEXT));
    }

    std::wstring shortcut;
    if (item.pinned) shortcut = L"Ctrl+" + item.pin;
    else {
        int number = 0;
        for (int i = 0; i <= index; ++i) if (!items[i].pinned) ++number;
        if (number <= 9) shortcut = L"Ctrl+" + std::to_wstring(number);
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
                                    const std::array<HWND, 4>& footerButtons) {
    auto it = std::find(footerButtons.begin(), footerButtons.end(), draw->hwndItem);
    const int index = it == footerButtons.end() ? -1 : static_cast<int>(it - footerButtons.begin());
    const bool selected = index >= 0 && index == activeFooter;
    FillRect(draw->hDC, &draw->rcItem, GetSysColorBrush(COLOR_WINDOW));
    if (selected || (draw->itemState & ODS_SELECTED)) {
        auto pen = SelectObject(draw->hDC, GetStockObject(NULL_PEN));
        auto brush = SelectObject(draw->hDC, GetSysColorBrush(selected ? COLOR_HIGHLIGHT : COLOR_BTNFACE));
        RoundRect(draw->hDC, 0, 0, draw->rcItem.right, draw->rcItem.bottom, 8, 8);
        SelectObject(draw->hDC, brush); SelectObject(draw->hDC, pen);
    }
    SelectObject(draw->hDC, m_normalFont);
    SetBkMode(draw->hDC, TRANSPARENT);
    SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
    RECT rect = draw->rcItem; rect.left += index < 0 ? 2 : 10; rect.right -= index < 0 ? 2 : 10;
    if (draw->CtlID == IDC_HISTORY_PREVIEW) {
        HPEN pen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
        auto oldPen = SelectObject(draw->hDC, pen);
        auto oldBrush = SelectObject(draw->hDC, GetStockObject(NULL_BRUSH));
        Rectangle(draw->hDC, 5, 5, 22, 19);
        MoveToEx(draw->hDC, 15, 5, nullptr); LineTo(draw->hDC, 15, 19);
        SelectObject(draw->hDC, oldBrush); SelectObject(draw->hDC, oldPen); DeleteObject(pen);
        return;
    }
    const auto title = draw->CtlID == IDC_HISTORY_SEARCH_CLEAR ? std::wstring(L"×") : ReadWindowText(draw->hwndItem);
    DrawTextW(draw->hDC, title.c_str(), -1, &rect, DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | (index < 0 ? DT_CENTER : DT_LEFT));
    if (index >= 0) {
        const wchar_t* keys[] = {(GetKeyState(VK_SHIFT) & 0x8000) ? L"Ctrl+Alt+Shift+Backspace" : L"Ctrl+Alt+Backspace", L"Ctrl+,", L"", L"Ctrl+Q"};
        SetTextColor(draw->hDC, GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_GRAYTEXT));
        DrawTextW(draw->hDC, keys[index], -1, &rect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    }
}

void HistoryRenderer::OnPaint(HDC dc, const RECT& client,
                             int pinSeparatorY,
                             int footerSeparatorY,
                             const SearchHeaderLayout::Geometry& header) {
    FillRect(dc, &client, GetSysColorBrush(COLOR_WINDOW));
    HPEN separator = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
    auto previousPen = SelectObject(dc, separator);
    for (int y : {pinSeparatorY, footerSeparatorY}) if (y >= 0) {
        MoveToEx(dc, 16, y, nullptr); LineTo(dc, client.right - 16, y);
    }
    SelectObject(dc, previousPen); DeleteObject(separator);
    if (header.showSearch) {
        auto pen = SelectObject(dc, GetStockObject(NULL_PEN));
        auto brush = SelectObject(dc, GetSysColorBrush(COLOR_BTNFACE));
        RoundRect(dc, header.search.left, header.search.top,
                  header.search.right, header.search.bottom,
                  header.metrics.cornerRadius, header.metrics.cornerRadius);
        SelectObject(dc, brush); SelectObject(dc, pen);
        HPEN iconPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_GRAYTEXT));
        pen = SelectObject(dc, iconPen); brush = SelectObject(dc, GetStockObject(NULL_BRUSH));
        const SearchHeaderLayout::IconGeometry icon = SearchHeaderLayout::IconFor(header);
        Ellipse(dc, icon.left, icon.top,
                icon.left + header.metrics.iconLensSize,
                icon.top + header.metrics.iconLensSize);
        MoveToEx(dc, icon.left + header.metrics.iconHandleStart,
                 icon.top + header.metrics.iconHandleStart, nullptr);
        LineTo(dc, icon.left + header.metrics.iconHandleEnd,
               icon.top + header.metrics.iconHandleEnd);
        SelectObject(dc, brush); SelectObject(dc, pen); DeleteObject(iconPen);
    }
    if (m_settings.show_title && header.showTitle && !IsRectEmpty(&header.title)) {
        const HFONT previous_font = static_cast<HFONT>(SelectObject(
            dc,
            m_smallFont != nullptr ? m_smallFont : m_normalFont
        ));
        const int previous_color = SetTextColor(dc, GetSysColor(COLOR_GRAYTEXT));
        const int previous_mode = SetBkMode(dc, TRANSPARENT);
        RECT title = header.title;
        DrawTextW(dc, L"maccy", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SetBkMode(dc, previous_mode);
        SetTextColor(dc, previous_color);
        SelectObject(dc, previous_font);
    }
}
