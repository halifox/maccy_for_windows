#pragma once

#include "PlatformConfig.h"

#include <atlbase.h>
#include <atlgdi.h>
#include <atlwin.h>
#include <commctrl.h>

#include <algorithm>
#include <climits>
#include <string>
#include <string_view>

// Small WTL/GDI helper for resource-backed text controls. Dialog templates
// cannot know the width of every localized string, so measure the active font
// and resize the control after its text changes.
class AutoSizeTextControl {
public:
    explicit AutoSizeTextControl(CWindow control) noexcept
        : m_control(control) {}

    bool SetText(
        std::wstring_view text,
        int minimum_width = 0,
        int maximum_width = 0,
        int horizontal_padding = 8
    ) {
        if (m_control.m_hWnd == nullptr || !::IsWindow(m_control.m_hWnd)) {
            return false;
        }

        const std::wstring value(text);
        if (!m_control.SetWindowText(value.c_str())) {
            return false;
        }
        return Resize(value, minimum_width, maximum_width, horizontal_padding);
    }

    bool Resize(
        std::wstring_view text,
        int minimum_width = 0,
        int maximum_width = 0,
        int horizontal_padding = 8
    ) {
        if (m_control.m_hWnd == nullptr || !::IsWindow(m_control.m_hWnd)) {
            return false;
        }

        const UINT dpi = ::GetDpiForWindow(m_control.m_hWnd);
        const int padding = MulDiv(std::max(0, horizontal_padding),
                                    static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
        const int text_width = MeasureWidth(text);
        int width = text_width + padding +
            MeasureControlChrome(dpi, text_width, padding);
        width = std::max(width, Scale(minimum_width, dpi));
        if (maximum_width > 0) {
            width = std::min(width, Scale(maximum_width, dpi));
        }

        RECT rect{};
        if (!m_control.GetWindowRect(&rect)) {
            return false;
        }
        HWND parent = ::GetParent(m_control.m_hWnd);
        if (parent != nullptr) {
            ::MapWindowPoints(nullptr, parent, reinterpret_cast<POINT *>(&rect), 2);
        }

        const int old_width = rect.right - rect.left;
        const LONG style = ::GetWindowLongW(m_control.m_hWnd, GWL_STYLE);
        if ((style & SS_TYPEMASK) == SS_RIGHT) {
            // Keep right-aligned labels anchored to the field that follows
            // them, so a longer translation grows to the left instead of
            // covering the next control.
            rect.left = rect.right - width;
        } else if ((style & SS_TYPEMASK) == SS_CENTER) {
            rect.left += (old_width - width) / 2;
        }

        return m_control.SetWindowPos(
            nullptr,
            rect.left,
            rect.top,
            std::max(1, width),
            std::max(1L, rect.bottom - rect.top),
            SWP_NOZORDER | SWP_NOACTIVATE
        ) != FALSE;
    }

    int MeasureWidth(std::wstring_view text) {
        if (m_control.m_hWnd == nullptr || !::IsWindow(m_control.m_hWnd)) {
            return 0;
        }

        CClientDC dc(m_control.m_hWnd);
        if (dc.m_hDC == nullptr) {
            return 0;
        }

        HFONT font = reinterpret_cast<HFONT>(
            ::SendMessageW(m_control.m_hWnd, WM_GETFONT, 0, 0)
        );
        if (font == nullptr) {
            font = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
        }
        const HGDIOBJ previous = ::SelectObject(dc.m_hDC, font);

        int widest = 0;
        std::size_t start = 0;
        while (start <= text.size()) {
            const std::size_t end = text.find_first_of(L"\r\n", start);
            const std::wstring_view line = text.substr(
                start,
                end == std::wstring_view::npos ? std::wstring_view::npos : end - start
            );
            if (!line.empty()) {
                const int length = static_cast<int>(std::min<std::size_t>(
                    line.size(), static_cast<std::size_t>(INT_MAX)
                ));
                SIZE size{};
                if (::GetTextExtentPoint32W(dc.m_hDC, line.data(), length, &size)) {
                    widest = std::max(widest, static_cast<int>(size.cx));
                }
            }

            if (end == std::wstring_view::npos) {
                break;
            }
            start = end + 1;
            if (text[end] == L'\r' && start < text.size() && text[start] == L'\n') {
                ++start;
            }
        }

        ::SelectObject(dc.m_hDC, previous);
        return widest;
    }

private:
    int MeasureControlChrome(UINT dpi, int text_width, int padding) const noexcept {
        wchar_t class_name[16]{};
        if (::GetClassNameW(m_control.m_hWnd, class_name, ARRAYSIZE(class_name)) == 0 ||
            std::wstring_view(class_name) != L"Button") {
            return 0;
        }

        const LONG style = ::GetWindowLongW(m_control.m_hWnd, GWL_STYLE);
        const LONG type = style & BS_TYPEMASK;
        switch (type) {
        case BS_CHECKBOX:
        case BS_AUTOCHECKBOX:
        case BS_3STATE:
        case BS_AUTO3STATE:
        case BS_RADIOBUTTON:
        case BS_AUTORADIOBUTTON:
            // The text measurement does not include the indicator drawn by a
            // checkbox/radio button. Without this space, BS_MULTILINE wraps
            // the last character even though the measured text fits.
            return ::GetSystemMetricsForDpi(SM_CXMENUCHECK, dpi) +
                Scale(4, dpi);
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON: {
            // The themed button border and minimum padding are not part of
            // GetTextExtentPoint32W. BCM_GETIDEALSIZE includes them for
            // push buttons and keeps short localized buttons usable.
            SIZE ideal{};
            if (Button_GetIdealSize(m_control.m_hWnd, &ideal) && ideal.cx > 0) {
                return std::max(0, static_cast<int>(ideal.cx) - text_width - padding);
            }
            return 0;
        }
        default:
            return 0;
        }
    }

    static int Scale(int value, UINT dpi) noexcept {
        return MulDiv(std::max(0, value), static_cast<int>(dpi), USER_DEFAULT_SCREEN_DPI);
    }

    CWindow m_control;
};
