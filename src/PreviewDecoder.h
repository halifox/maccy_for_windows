#pragma once

#include "PlatformConfig.h"

#include <optional>
#include <thread>
#include <vector>

#include "ClipboardData.h"

struct PreviewBitmap {
    HBITMAP handle = nullptr;
    int width = 0;
    int height = 0;

    PreviewBitmap() = default;
    PreviewBitmap(HBITMAP value, int bitmap_width, int bitmap_height)
        : handle(value), width(bitmap_width), height(bitmap_height) {}

    ~PreviewBitmap();

    PreviewBitmap(const PreviewBitmap &) = delete;
    PreviewBitmap &operator=(const PreviewBitmap &) = delete;

    PreviewBitmap(PreviewBitmap &&other) noexcept;
    PreviewBitmap &operator=(PreviewBitmap &&other) noexcept;

    HBITMAP Release() noexcept;
};

std::optional<PreviewBitmap> DecodePreviewBitmap(
    const ClipboardItem &item,
    UINT maximum_width,
    UINT maximum_height,
    std::stop_token stop_token
);
