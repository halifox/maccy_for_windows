#define NOMINMAX
#include "PreviewWindow.h"

#include <wincodec.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include <atlbase.h>

namespace {

bool IsUnicodeText(const ClipboardFormatData &data) {
    return data.format == CF_UNICODETEXT || data.name == L"CF_UNICODETEXT";
}

bool IsAnsiText(const ClipboardFormatData &data) {
    return data.format == CF_TEXT || data.name == L"CF_TEXT";
}

std::wstring FullText(const ClipboardItem &item) {
    for (const ClipboardFormatData &data : item.data) {
        if (!IsUnicodeText(data) || data.bytes.size() < sizeof(wchar_t)) {
            continue;
        }
        const auto *text = reinterpret_cast<const wchar_t *>(data.bytes.data());
        const size_t count = data.bytes.size() / sizeof(wchar_t);
        size_t length = 0;
        while (length < count && text[length] != L'\0') {
            ++length;
        }
        return std::wstring(text, length);
    }

    for (const ClipboardFormatData &data : item.data) {
        if (!IsAnsiText(data) || data.bytes.empty()) {
            continue;
        }
        const int source_length = static_cast<int>(
            std::min<size_t>(data.bytes.size(), static_cast<size_t>(INT_MAX))
        );
        const int length = MultiByteToWideChar(
            CP_ACP,
            0,
            reinterpret_cast<const char *>(data.bytes.data()),
            source_length,
            nullptr,
            0
        );
        if (length <= 0) {
            continue;
        }
        std::wstring result(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(
            CP_ACP,
            0,
            reinterpret_cast<const char *>(data.bytes.data()),
            source_length,
            result.data(),
            length
        );
        const size_t nul = result.find(L'\0');
        if (nul != std::wstring::npos) {
            result.resize(nul);
        }
        return result;
    }

    return item.content;
}

HBITMAP CreateBitmapFromDib(const std::vector<unsigned char> &bytes) {
    if (bytes.size() < sizeof(BITMAPINFOHEADER)) {
        return nullptr;
    }

    const auto *header = reinterpret_cast<const BITMAPINFOHEADER *>(bytes.data());
    if (header->biSize < sizeof(BITMAPINFOHEADER) ||
        header->biSize > bytes.size() ||
        header->biWidth <= 0 || header->biHeight == 0 ||
        header->biPlanes != 1 ||
        (header->biBitCount != 1 && header->biBitCount != 4 &&
         header->biBitCount != 8 && header->biBitCount != 16 &&
         header->biBitCount != 24 && header->biBitCount != 32)) {
        return nullptr;
    }

    size_t bits_offset = header->biSize;
    if (header->biBitCount <= 8) {
        const DWORD colors = header->biClrUsed != 0
            ? header->biClrUsed
            : (1u << header->biBitCount);
        bits_offset += static_cast<size_t>(colors) * sizeof(RGBQUAD);
    } else if (header->biCompression == BI_BITFIELDS &&
               header->biSize == sizeof(BITMAPINFOHEADER)) {
        bits_offset += 3 * sizeof(DWORD);
    }
    if (bits_offset >= bytes.size()) {
        return nullptr;
    }

    void *destination = nullptr;
    HBITMAP bitmap = ::CreateDIBSection(
        nullptr,
        reinterpret_cast<const BITMAPINFO *>(bytes.data()),
        DIB_RGB_COLORS,
        &destination,
        nullptr,
        0
    );
    if (bitmap == nullptr || destination == nullptr) {
        if (bitmap != nullptr) {
            ::DeleteObject(bitmap);
        }
        return nullptr;
    }

    const size_t width = static_cast<size_t>(header->biWidth);
    const size_t height = static_cast<size_t>(
        header->biHeight < 0 ? -static_cast<LONGLONG>(header->biHeight) : header->biHeight
    );
    const size_t row_bytes = ((width * header->biBitCount + 31u) / 32u) * 4u;
    const size_t image_bytes = row_bytes * height;
    const size_t available = bytes.size() - bits_offset;
    std::memcpy(destination, bytes.data() + bits_offset, std::min(image_bytes, available));
    return bitmap;
}

HBITMAP CreateBitmapFromEncoded(const std::vector<unsigned char> &bytes) {
    if (bytes.empty()) {
        return nullptr;
    }

    CComPtr<IWICImagingFactory> factory;
    if (FAILED(::CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    ))) {
        return nullptr;
    }

    HGLOBAL memory = ::GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (memory == nullptr) {
        return nullptr;
    }
    void *destination = ::GlobalLock(memory);
    if (destination == nullptr) {
        ::GlobalFree(memory);
        return nullptr;
    }
    std::memcpy(destination, bytes.data(), bytes.size());
    ::GlobalUnlock(memory);

    CComPtr<IStream> stream;
    if (FAILED(::CreateStreamOnHGlobal(memory, TRUE, &stream))) {
        ::GlobalFree(memory);
        return nullptr;
    }

    CComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(
        stream,
        nullptr,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    ))) {
        return nullptr;
    }

    CComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return nullptr;
    }

    CComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(
            frame,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom
        ))) {
        return nullptr;
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
        return nullptr;
    }

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = static_cast<LONG>(width);
    info.bmiHeader.biHeight = -static_cast<LONG>(height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    void *bits = nullptr;
    HBITMAP bitmap = ::CreateDIBSection(
        nullptr,
        &info,
        DIB_RGB_COLORS,
        &bits,
        nullptr,
        0
    );
    if (bitmap == nullptr || bits == nullptr) {
        if (bitmap != nullptr) {
            ::DeleteObject(bitmap);
        }
        return nullptr;
    }

    const UINT stride = width * 4u;
    const UINT buffer_size = stride * height;
    if (FAILED(converter->CopyPixels(nullptr, stride, buffer_size, static_cast<BYTE *>(bits)))) {
        ::DeleteObject(bitmap);
        return nullptr;
    }
    return bitmap;
}

HBITMAP CreateBitmapForItem(const ClipboardItem &item) {
    for (const ClipboardFormatData &data : item.data) {
        if (data.format == CF_DIBV5 || data.format == CF_DIB) {
            if (HBITMAP bitmap = CreateBitmapFromDib(data.bytes)) {
                return bitmap;
            }
        }
    }
    for (const ClipboardFormatData &data : item.data) {
        if (HBITMAP bitmap = CreateBitmapFromEncoded(data.bytes)) {
            return bitmap;
        }
    }
    return nullptr;
}

} // namespace

bool PreviewWindow::Initialize(HWND owner) {
    return Create(owner) != nullptr;
}

void PreviewWindow::ClearBitmap() {
    if (m_bitmap != nullptr) {
        ::DeleteObject(m_bitmap);
        m_bitmap = nullptr;
    }
    m_bitmapWidth = 0;
    m_bitmapHeight = 0;
}

bool PreviewWindow::LoadBitmapForItem(const ClipboardItem &item) {
    ClearBitmap();
    if (!item.has_image) {
        return false;
    }
    m_bitmap = CreateBitmapForItem(item);
    if (m_bitmap == nullptr) {
        return false;
    }
    BITMAP bitmap{};
    if (::GetObjectW(m_bitmap, sizeof(bitmap), &bitmap) != sizeof(bitmap)) {
        ClearBitmap();
        return false;
    }
    m_bitmapWidth = bitmap.bmWidth;
    m_bitmapHeight = std::abs(bitmap.bmHeight);
    return m_bitmapWidth > 0 && m_bitmapHeight > 0;
}

void PreviewWindow::UpdateStatus(const ClipboardItem &item, bool image_loaded) {
    if (m_status == nullptr) {
        return;
    }
    std::wstring status;
    if (image_loaded) {
        status = L"图片 " + std::to_wstring(m_bitmapWidth) + L" × " +
            std::to_wstring(m_bitmapHeight);
    } else if (item.has_image) {
        status = L"无法解码图片，显示文本摘要";
    } else if (item.has_files) {
        status = L"文件项目";
    } else {
        status = L"完整文本预览";
    }
    ::SetWindowTextW(m_status, status.c_str());
}

void PreviewWindow::SetItem(const ClipboardItem &item) {
    const bool image_loaded = LoadBitmapForItem(item);
    const std::wstring text = FullText(item);
    ::SetWindowTextW(m_hWnd, L"剪贴板预览");
    ::SetWindowTextW(m_text, text.c_str());
    ::SendMessageW(m_text, EM_SETSEL, 0, 0);
    ::ShowWindow(m_image, image_loaded ? SW_SHOW : SW_HIDE);
    ::ShowWindow(m_text, image_loaded ? SW_HIDE : SW_SHOW);
    UpdateStatus(item, image_loaded);
    ::InvalidateRect(m_image, nullptr, TRUE);
}

void PreviewWindow::Hide() {
    if (m_hWnd != nullptr) {
        ::ShowWindow(m_hWnd, SW_HIDE);
    }
}

bool PreviewWindow::IsVisible() const noexcept {
    return m_hWnd != nullptr && ::IsWindowVisible(m_hWnd) != FALSE;
}

bool PreviewWindow::ContainsWindow(HWND window) const noexcept {
    return window != nullptr && m_hWnd != nullptr &&
        (window == m_hWnd || ::IsChild(m_hWnd, window) != FALSE);
}

LRESULT PreviewWindow::OnInitDialog(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    m_image = ::GetDlgItem(m_hWnd, IDC_PREVIEW_IMAGE);
    m_text = ::GetDlgItem(m_hWnd, IDC_PREVIEW_TEXT);
    m_status = ::GetDlgItem(m_hWnd, IDC_PREVIEW_STATUS);
    const HFONT font = static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT));
    for (HWND control : {m_image, m_text, m_status}) {
        if (control != nullptr) {
            ::SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
        }
    }
    ::SendMessageW(m_text, EM_SETLIMITTEXT, 4 * 1024 * 1024, 0);
    ::ShowWindow(m_image, SW_HIDE);
    ::ShowWindow(m_text, SW_HIDE);
    return TRUE;
}

LRESULT PreviewWindow::OnClose(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    Hide();
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
    if (m_bitmap == nullptr) {
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

    HDC source = ::CreateCompatibleDC(draw->hDC);
    if (source == nullptr) {
        return 0;
    }
    HGDIOBJ previous = ::SelectObject(source, m_bitmap);
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
    ::SelectObject(source, previous);
    ::DeleteDC(source);
    ::FrameRect(draw->hDC, &draw->rcItem, ::GetSysColorBrush(COLOR_GRAYTEXT));
    return 0;
}

LRESULT PreviewWindow::OnDestroy(UINT, WPARAM, LPARAM, BOOL &handled) {
    handled = TRUE;
    ClearBitmap();
    m_image = nullptr;
    m_text = nullptr;
    m_status = nullptr;
    m_hWnd = nullptr;
    return 0;
}
