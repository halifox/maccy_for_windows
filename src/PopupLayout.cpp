#include "PopupLayout.h"

#include <algorithm>

namespace {

constexpr int kWindowMargin = 5;
constexpr int kSearchGap = 6;
constexpr int kSearchIconWidth = 24;
constexpr int kSearchClearWidth = 20;
constexpr int kPreviewWidth = 23;
constexpr int kHeaderGap = 6;
constexpr int kItemHeight = 22;
constexpr int kFooterHeight = 22;
constexpr int kFooterGap = 6;
constexpr int kSectionGap = 6;
constexpr int kFooterCount = 4;

} // namespace

PopupLayoutResult PopupLayout::Calculate(const PopupLayoutInput& input) {
    PopupLayoutResult result;
    const int margin = kWindowMargin;
    const int width = std::max(1L, input.client.right - 2 * margin);
    const int headerHeight = input.searchVisible ? input.searchHeight : 0;

    result.showTitle = input.searchVisible && input.titleWidth > 0;
    result.title = {margin, margin, margin + input.titleWidth, margin + headerHeight};

    const int previewLeft = margin + width - kPreviewWidth;
    const int searchLeft = margin + input.titleWidth + (input.titleWidth ? kHeaderGap : 0);
    const int searchRight = previewLeft - kHeaderGap;
    result.search = {searchLeft, margin, searchRight, margin + headerHeight};
    result.searchEdit = {
        searchLeft + kSearchIconWidth,
        margin,
        std::max<LONG>(searchLeft + kSearchIconWidth + 1,
                       searchRight - kSearchClearWidth),
        margin + input.searchHeight
    };
    result.searchClear = {
        searchRight - kSearchClearWidth,
        margin,
        searchRight,
        margin + input.searchHeight
    };
    result.previewToggle = {previewLeft, margin, previewLeft + kPreviewWidth, margin + input.searchHeight};
    result.showSearchClear = input.searchVisible;
    result.showPreviewToggle = input.searchVisible;

    const int top = margin + (input.searchVisible ? headerHeight + kSearchGap : 0);
    const int footerHeight = input.showFooter
        ? kFooterGap + kFooterHeight * kFooterCount
        : 0;
    const int bottom = std::max(top + 1, static_cast<int>(input.client.bottom) - margin - footerHeight);
    const int available = std::max(1, bottom - top);
    const bool havePins = input.pinCount > 0;
    const bool haveHistory = input.historyCount > 0;
    const int gap = havePins && haveHistory ? kSectionGap : 0;
    const int requestedPinsHeight = input.pinCount * kItemHeight;
    const int pinsHeight = havePins
        ? std::min(requestedPinsHeight, haveHistory
            ? std::max(1, available - gap - kItemHeight)
            : available)
        : 0;
    const int historyHeight = haveHistory ? std::max(1, available - pinsHeight - gap) : 1;

    const int pinTop = input.pinsAtBottom && haveHistory ? top + historyHeight + gap : top;
    const int historyTop = !input.pinsAtBottom && havePins ? top + pinsHeight + gap : top;
    result.pins = {margin, pinTop, margin + width, pinTop + pinsHeight};
    result.history = {margin, historyTop, margin + width, historyTop + historyHeight};
    result.showPins = havePins;
    result.showHistory = haveHistory || !havePins;
    result.pinSeparatorY = gap ? (input.pinsAtBottom ? pinTop - gap / 2 : historyTop - gap / 2) : -1;
    result.footerSeparatorY = input.showFooter ? bottom + 5 : -1;
    for (int index = 0; index < kFooterCount; ++index) {
        result.footer[static_cast<size_t>(index)] = {
            margin,
            bottom + kFooterGap + index * kFooterHeight,
            margin + width,
            bottom + kFooterGap + (index + 1) * kFooterHeight
        };
    }
    return result;
}
