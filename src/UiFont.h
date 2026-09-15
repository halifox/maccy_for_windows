#pragma once

#include "PlatformConfig.h"

namespace UiFont {

constexpr int kBodyPointSize = 9;
constexpr int kSmallPointSize = 7;

inline UINT DpiForWindow(HWND window) {
    if (window != nullptr) {
        if (HDC dc = ::GetDC(window)) {
            const int dpi = ::GetDeviceCaps(dc, LOGPIXELSY);
            ::ReleaseDC(window, dc);
            if (dpi > 0) {
                return static_cast<UINT>(dpi);
            }
        }
    }
    return USER_DEFAULT_SCREEN_DPI;
}

inline HFONT CreateSegoeUi(UINT dpi, int pointSize, LONG weight = FW_NORMAL,
                          BOOL italic = FALSE, BOOL underline = FALSE) {
    LOGFONTW font{};
    font.lfHeight = -MulDiv(pointSize, static_cast<int>(dpi), 72);
    font.lfWeight = weight;
    font.lfItalic = italic;
    font.lfUnderline = underline;
    font.lfCharSet = DEFAULT_CHARSET;
    font.lfQuality = CLEARTYPE_QUALITY;
    wcscpy_s(font.lfFaceName, L"Segoe UI");
    return ::CreateFontIndirectW(&font);
}

} // namespace UiFont
