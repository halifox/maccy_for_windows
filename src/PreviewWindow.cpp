#include "PreviewWindow.h"
#include "ClipboardRules.h"
#include "Constants.h"
#include "UiFont.h"

#include <dwmapi.h>

#include <algorithm>
#include <cmath>
#include <ctime>
#include <string>

namespace {

constexpr UINT kFallbackPreviewWidth = 194;
constexpr UINT kFallbackPreviewHeight = 98;

void ApplySystemRoundedCorners(HWND window) {
    const DWM_WINDOW_CORNER_PREFERENCE preference = DWMWCP_ROUND;
    ::DwmSetWindowAttribute(
        window,
        DWMWA_WINDOW_CORNER_PREFERENCE,
        &preference,
        sizeof(preference)
    );
}

std::wstring FormatCopyTime(sqlite3_int64 milliseconds) {
    if (milliseconds <= 0) {
        return L"未知";
    }

    const time_t seconds = static_cast<time_t>(milliseconds / 1000);
    tm local_time{};
    if (::localtime_s(&local_time, &seconds) != 0) {
        return L"未知";
    }

    wchar_t buffer[64]{};
    if (::wcsftime(buffer, ARRAYSIZE(buffer), L"%Y-%m-%d %H:%M:%S", &local_time) == 0) {
        return L"未知";
    }
    return buffer;
}

std::wstring FormatApplication(std::wstring_view application) {
    if (application.empty()) {
        return L"未知";
    }
    const size_t separator = application.find_last_of(L"\\/");
    if (separator == std::wstring_view::npos || separator + 1 >= application.size()) {
        return std::wstring(application);
    }
    return std::wstring(application.substr(separator + 1));
}

} // namespace

bool PreviewWindow::Initialize(HWND owner) {
    return Create(owner) != nullptr;
}

void PreviewWindow::ClearBitmap() {
    if (!m_bitmap.IsNull()) {
        m_bitmap.DeleteObject();
    }
    m_bitmapWidth = 0;
    m_bitmapHeight = 0;
}

void PreviewWindow::GetImageSize(UINT &width, UINT &height) const noexcept {
    RECT client{};
    if (m_image.m_hWnd != nullptr && m_image.GetClientRect(&client)) {
        width = client.right > client.left
            ? static_cast<UINT>(client.right - client.left)
            : kFallbackPreviewWidth;
        height = client.bottom > client.top
            ? static_cast<UINT>(client.bottom - client.top)
            : kFallbackPreviewHeight;
        return;
    }
    width = kFallbackPreviewWidth;
    height = kFallbackPreviewHeight;
}

void PreviewWindow::UpdateStatus(const ClipboardItem &item) {
    if (m_status.m_hWnd == nullptr) {
        return;
    }

    std::wstring status = L"应用来源：" + FormatApplication(item.application);
    status += L"\r\n第一次复制时间：" + FormatCopyTime(item.first_copied_at);
    status += L"\r\n最后一次复制时间：" + FormatCopyTime(item.copied_at);
    status += L"\r\n复制次数：" + std::to_wstring(std::max(1, item.copy_count));
    m_status.SetWindowText(status.c_str());
}

void PreviewWindow::SetItem(const ClipboardItem &item, std::wstring text, PreviewBitmap bitmap) {
    m_itemId = item.id;
    m_pinButton.SetWindowText(item.pinned ? L"取消置顶" : L"置顶");
    ClearBitmap();
    m_bitmapWidth = bitmap.width;
    m_bitmapHeight = bitmap.height;
    m_bitmap.Attach(bitmap.Release());
    const bool image_loaded = !m_bitmap.IsNull() && m_bitmapWidth > 0 && m_bitmapHeight > 0;
    m_text.SetWindowText(text.c_str());
    m_text.SetSel(0, 0);
    m_image.ShowWindow(image_loaded ? SW_SHOW : SW_HIDE);
    m_text.ShowWindow(image_loaded ? SW_HIDE : SW_SHOW);
    UpdateStatus(item);
    m_image.InvalidateRect(nullptr, TRUE);
}

void PreviewWindow::Hide() {
    if (m_hWnd != nullptr) {
        ClearBitmap();
        if (m_text.m_hWnd != nullptr) {
            m_text.SetWindowText(L"");
        }
        if (m_status.m_hWnd != nullptr) {
            m_status.SetWindowText(L"");
        }
        m_image.ShowWindow(SW_HIDE);
        m_text.ShowWindow(SW_HIDE);
        ShowWindow(SW_HIDE);
        m_itemId = 0;
    }
}

bool PreviewWindow::IsVisible() const noexcept {
    return m_hWnd != nullptr && ::IsWindowVisible(m_hWnd) != FALSE;
}

LRESULT PreviewWindow::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    ApplySystemRoundedCorners(m_hWnd);
    m_image = GetDlgItem(IDC_PREVIEW_IMAGE);
    m_text = GetDlgItem(IDC_PREVIEW_TEXT);
    m_status = GetDlgItem(IDC_PREVIEW_STATUS);
    m_pinButton = GetDlgItem(IDC_PREVIEW_PIN);
    m_deleteButton = GetDlgItem(IDC_PREVIEW_DELETE);
    UpdateFont(UiFont::DpiForWindow(m_hWnd));
    m_text.SetLimitText(ClipboardRules::Limits::kMaximumPreviewTextCharacters);
    m_image.ShowWindow(SW_HIDE);
    m_text.ShowWindow(SW_HIDE);
    return TRUE;
}

bool PreviewWindow::UpdateFont(UINT dpi) {
    HFONT font = UiFont::CreateSegoeUi(dpi, UiFont::kBodyPointSize);
    if (font == nullptr) return false;
    m_image.SetFont(font, TRUE);
    m_text.SetFont(font, TRUE);
    m_status.SetFont(font, TRUE);
    m_pinButton.SetFont(font, TRUE);
    m_deleteButton.SetFont(font, TRUE);
    m_font.DeleteObject();
    m_font.Attach(font);
    return true;
}

LRESULT PreviewWindow::OnDpiChanged(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    handled = TRUE;
    UpdateFont(HIWORD(wParam));
    return 0;
}

LRESULT PreviewWindow::OnActivate(UINT, WPARAM wParam, LPARAM lParam, BOOL &handled) {
    if (LOWORD(wParam) != WA_INACTIVE) {
        handled = FALSE;
        return 0;
    }

    handled = TRUE;
    CWindow owner(::GetWindow(m_hWnd, GW_OWNER));
    if (owner.IsWindow()) {
        owner.PostMessage(AppConstants::kPopupActivationMessage, wParam, lParam);
    }
    return 0;
}

LRESULT PreviewWindow::OnClose(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    CWindow owner(::GetWindow(m_hWnd, GW_OWNER));
    if (owner.IsWindow()) {
        owner.PostMessage(WM_COMMAND, MAKEWPARAM(IDC_HISTORY_PREVIEW, BN_CLICKED), 0);
    }
    return 0;
}

LRESULT PreviewWindow::OnCommand(WORD, WORD id, HWND, BOOL &handled) {
    handled = id == IDC_PREVIEW_PIN || id == IDC_PREVIEW_DELETE;
    CWindow owner(::GetWindow(m_hWnd, GW_OWNER));
    if (handled && owner.IsWindow() && m_itemId != 0) {
        owner.PostMessage(
            WM_COMMAND,
            MAKEWPARAM(id, BN_CLICKED),
            static_cast<LPARAM>(m_itemId)
        );
    }
    return 0;
}

LRESULT PreviewWindow::OnDrawItem(UINT, WPARAM, LPARAM lParam, BOOL &handled) {
    auto *draw = reinterpret_cast<DRAWITEMSTRUCT *>(lParam);
    if (draw == nullptr || draw->CtlID != IDC_PREVIEW_IMAGE) {
        handled = FALSE;
        return 0;
    }
    handled = TRUE;
    ::FillRect(draw->hDC, &draw->rcItem, ::GetSysColorBrush(COLOR_WINDOW));
    if (m_bitmap.IsNull()) {
        ::SetBkMode(draw->hDC, TRANSPARENT);
        ::DrawTextW(
            draw->hDC,
            L"无法显示图片",
            -1,
            &draw->rcItem,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE
        );
        return 0;
    }

    const int available_width = std::max(1L, draw->rcItem.right - draw->rcItem.left - 16L);
    const int available_height = std::max(1L, draw->rcItem.bottom - draw->rcItem.top - 16L);
    const double scale = std::min(
        static_cast<double>(available_width) / m_bitmapWidth,
        static_cast<double>(available_height) / m_bitmapHeight
    );
    const int width = std::max(1, static_cast<int>(std::lround(m_bitmapWidth * scale)));
    const int height = std::max(1, static_cast<int>(std::lround(m_bitmapHeight * scale)));
    const int left = draw->rcItem.left + (draw->rcItem.right - draw->rcItem.left - width) / 2;
    const int top = draw->rcItem.top + (draw->rcItem.bottom - draw->rcItem.top - height) / 2;

    CDC source;
    if (!source.CreateCompatibleDC(draw->hDC)) {
        return 0;
    }
    HBITMAP previous = source.SelectBitmap(m_bitmap);
    ::SetStretchBltMode(draw->hDC, HALFTONE);
    ::SetBrushOrgEx(draw->hDC, 0, 0, nullptr);
    ::StretchBlt(
        draw->hDC,
        left,
        top,
        width,
        height,
        source,
        0,
        0,
        m_bitmapWidth,
        m_bitmapHeight,
        SRCCOPY
    );
    source.SelectBitmap(previous);
    ::FrameRect(draw->hDC, &draw->rcItem, ::GetSysColorBrush(COLOR_GRAYTEXT));
    return 0;
}

LRESULT PreviewWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    ClearBitmap();
    m_font.DeleteObject();
    m_image.m_hWnd = nullptr;
    m_text.m_hWnd = nullptr;
    m_status.m_hWnd = nullptr;
    m_pinButton.m_hWnd = nullptr;
    m_deleteButton.m_hWnd = nullptr;
    m_itemId = 0;
    m_hWnd = nullptr;
    return 0;
}

LRESULT PreviewWindow::OnControlColor(UINT, WPARAM wParam, LPARAM, BOOL &handled) {
    handled = TRUE;
    HDC dc = reinterpret_cast<HDC>(wParam);
    SetTextColor(dc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(dc, GetSysColor(COLOR_WINDOW));
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
}
