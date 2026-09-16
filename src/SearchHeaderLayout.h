#pragma once

#include "PlatformConfig.h"

#include <algorithm>

namespace SearchHeaderLayout {

constexpr int kWindowMargin = 5;
constexpr int kHeight = 23;
constexpr int kGap = 6;
constexpr int kIconSlotWidth = 24;
constexpr int kClearWidth = 20;
constexpr int kPreviewWidth = 23;
constexpr int kIconLeftInset = 7;
constexpr int kIconLensSize = 9;
constexpr int kIconHandleStart = 7;
constexpr int kIconHandleEnd = 13;
constexpr int kIconVisualHeight = 13;
constexpr int kCornerRadius = 8;
constexpr int kTitlePadding = 8;

inline int Scale(int value, UINT dpi) {
    const UINT effectiveDpi = dpi == 0 ? USER_DEFAULT_SCREEN_DPI : dpi;
    return std::max(1, MulDiv(value, static_cast<int>(effectiveDpi),
                              USER_DEFAULT_SCREEN_DPI));
}

struct Metrics {
    int windowMargin = kWindowMargin;
    int height = kHeight;
    int contentGap = kGap;
    int headerGap = kGap;
    int iconSlotWidth = kIconSlotWidth;
    int clearWidth = kClearWidth;
    int previewWidth = kPreviewWidth;
    int iconLeftInset = kIconLeftInset;
    int iconLensSize = kIconLensSize;
    int iconHandleStart = kIconHandleStart;
    int iconHandleEnd = kIconHandleEnd;
    int iconVisualHeight = kIconVisualHeight;
    int cornerRadius = kCornerRadius;
    int titlePadding = kTitlePadding;
};

inline Metrics ForDpi(UINT dpi) {
    return {
        Scale(kWindowMargin, dpi),
        Scale(kHeight, dpi),
        Scale(kGap, dpi),
        Scale(kGap, dpi),
        Scale(kIconSlotWidth, dpi),
        Scale(kClearWidth, dpi),
        Scale(kPreviewWidth, dpi),
        Scale(kIconLeftInset, dpi),
        Scale(kIconLensSize, dpi),
        Scale(kIconHandleStart, dpi),
        Scale(kIconHandleEnd, dpi),
        Scale(kIconVisualHeight, dpi),
        Scale(kCornerRadius, dpi),
        Scale(kTitlePadding, dpi)
    };
}

struct Geometry {
    RECT title{};
    RECT search{};
    RECT searchEdit{};
    RECT searchClear{};
    RECT preview{};
    Metrics metrics{};
    bool showSearch = false;
    bool showTitle = false;
};

struct IconGeometry {
    int left = 0;
    int top = 0;
};

inline Geometry Calculate(const RECT& client, int titleWidth, bool visible,
                          const Metrics& metrics) {
    const int width = std::max(1, static_cast<int>(client.right) -
        2 * metrics.windowMargin);
    const int height = visible ? metrics.height : 0;
    const int previewLeft = metrics.windowMargin + width - metrics.previewWidth;
    const int searchLeft = metrics.windowMargin + titleWidth +
        (titleWidth > 0 ? metrics.headerGap : 0);
    const int searchRight = previewLeft - metrics.headerGap;
    const int searchEditLeft = searchLeft + metrics.iconSlotWidth;
    const int searchEditRight = std::max(searchEditLeft + 1,
                                         searchRight - metrics.clearWidth);

    Geometry result{};
    result.title = {metrics.windowMargin, metrics.windowMargin,
                    metrics.windowMargin + titleWidth, metrics.windowMargin + height};
    result.search = {searchLeft, metrics.windowMargin, searchRight,
                     metrics.windowMargin + height};
    result.searchEdit = {searchEditLeft, metrics.windowMargin, searchEditRight,
                         metrics.windowMargin + height};
    result.searchClear = {searchRight - metrics.clearWidth, metrics.windowMargin,
                          searchRight, metrics.windowMargin + height};
    result.preview = {previewLeft, metrics.windowMargin,
                      previewLeft + metrics.previewWidth, metrics.windowMargin + height};
    result.metrics = metrics;
    result.showSearch = visible;
    result.showTitle = visible && titleWidth > 0;
    return result;
}

inline Geometry Calculate(const RECT& client, int titleWidth, bool visible, UINT dpi) {
    return Calculate(client, titleWidth, visible, ForDpi(dpi));
}

inline IconGeometry IconFor(const Geometry& header) {
    const RECT& search = header.search;
    const int height = std::max(1, static_cast<int>(search.bottom - search.top));
    return {
        search.left + header.metrics.iconLeftInset,
        search.top + std::max(0, (height - header.metrics.iconVisualHeight) / 2)
    };
}

} // namespace SearchHeaderLayout
