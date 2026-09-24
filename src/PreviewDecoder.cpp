#include "PreviewDecoder.h"
#include "ClipboardRules.h"
#include "GdiScope.h"
#include "Win32Resources.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlgdi.h>
#include <wincodec.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cwctype>
#include <cstring>
#include <limits>
#include <utility>

namespace {

bool FitSize(
    UINT source_width,
    UINT source_height,
    UINT maximum_width,
    UINT maximum_height,
    UINT &width,
    UINT &height
) {
    if (source_width == 0 || source_height == 0 || maximum_width == 0 || maximum_height == 0) {
        return false;
    }

    const double scale = std::min({
        1.0,
        static_cast<double>(maximum_width) / source_width,
        static_cast<double>(maximum_height) / source_height,
    });
    width = std::max(1u, static_cast<UINT>(std::floor(source_width * scale)));
    height = std::max(1u, static_cast<UINT>(std::floor(source_height * scale)));
    return static_cast<std::uint64_t>(width) * height <= ClipboardRules::Limits::kMaximumImagePixels;
}

bool ReadDibLayout(
    const std::vector<unsigned char> &bytes,
    BITMAPINFOHEADER &header,
    size_t &bits_offset,
    UINT &width,
    UINT &height
) {
    if (bytes.size() < sizeof(BITMAPINFOHEADER)) {
        return false;
    }
    std::memcpy(&header, bytes.data(), sizeof(header));
    if (header.biSize < sizeof(BITMAPINFOHEADER) ||
        header.biSize > bytes.size() ||
        header.biWidth <= 0 || header.biHeight == 0 ||
        header.biPlanes != 1 ||
        (header.biBitCount != 1 && header.biBitCount != 4 &&
         header.biBitCount != 8 && header.biBitCount != 16 &&
         header.biBitCount != 24 && header.biBitCount != 32) ||
        (header.biCompression != BI_RGB && header.biCompression != BI_BITFIELDS)) {
        return false;
    }

    std::uint64_t offset = header.biSize;
    if (header.biBitCount <= 8) {
        const std::uint64_t colors = header.biClrUsed != 0
            ? header.biClrUsed
            : (1ULL << header.biBitCount);
        offset += colors * sizeof(RGBQUAD);
    } else if (header.biCompression == BI_BITFIELDS &&
               header.biSize == sizeof(BITMAPINFOHEADER)) {
        offset += 3 * sizeof(DWORD);
    }
    if (offset >= bytes.size()) {
        return false;
    }

    const std::uint64_t source_width = static_cast<std::uint64_t>(header.biWidth);
    const std::uint64_t source_height = static_cast<std::uint64_t>(
        header.biHeight < 0 ? -static_cast<LONGLONG>(header.biHeight) : header.biHeight
    );
    if (source_width == 0 || source_height == 0 ||
        source_width * source_height > ClipboardRules::Limits::kMaximumImagePixels) {
        return false;
    }

    const std::uint64_t row_bits = source_width * header.biBitCount;
    const std::uint64_t row_bytes = ((row_bits + 31) / 32) * 4;
    const std::uint64_t image_bytes = row_bytes * source_height;
    if (image_bytes > bytes.size() - offset) {
        return false;
    }

    bits_offset = static_cast<size_t>(offset);
    width = static_cast<UINT>(source_width);
    height = static_cast<UINT>(source_height);
    return true;
}

HBITMAP CreateOutputBitmap(UINT width, UINT height, void **bits) {
    if (width == 0 || height == 0 ||
        static_cast<std::uint64_t>(width) * height > ClipboardRules::Limits::kMaximumImagePixels) {
        return nullptr;
    }
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = static_cast<LONG>(width);
    info.bmiHeader.biHeight = -static_cast<LONG>(height);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    return ::CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, bits, nullptr, 0);
}

std::optional<PreviewBitmap> CreateBitmapFromDib(
    const std::vector<unsigned char> &bytes,
    UINT maximum_width,
    UINT maximum_height,
    std::stop_token stop_token
) {
    BITMAPINFOHEADER header{};
    size_t bits_offset = 0;
    UINT source_width = 0;
    UINT source_height = 0;
    if (!ReadDibLayout(bytes, header, bits_offset, source_width, source_height)) {
        return std::nullopt;
    }

    UINT width = 0;
    UINT height = 0;
    if (!FitSize(source_width, source_height, maximum_width, maximum_height, width, height)) {
        return std::nullopt;
    }

    void *destination = nullptr;
    CBitmap bitmap;
    bitmap.Attach(CreateOutputBitmap(width, height, &destination));
    if (bitmap.IsNull() || destination == nullptr) {
        return std::nullopt;
    }

    if (stop_token.stop_requested()) {
        return std::nullopt;
    }

    CDC dc;
    if (!dc.CreateCompatibleDC(nullptr)) {
        return std::nullopt;
    }
    ScopedGdiObjectSelection selected_bitmap(dc.m_hDC, static_cast<HBITMAP>(bitmap));
    if (!selected_bitmap.IsSelected()) {
        return std::nullopt;
    }
    ::SetStretchBltMode(dc.m_hDC, HALFTONE);
    ::SetBrushOrgEx(dc.m_hDC, 0, 0, nullptr);
    const int result = ::StretchDIBits(
        dc.m_hDC,
        0,
        0,
        static_cast<int>(width),
        static_cast<int>(height),
        0,
        0,
        static_cast<int>(source_width),
        static_cast<int>(source_height),
        bytes.data() + bits_offset,
        reinterpret_cast<const BITMAPINFO *>(bytes.data()),
        DIB_RGB_COLORS,
        SRCCOPY
    );
    if (result == GDI_ERROR || stop_token.stop_requested()) {
        return std::nullopt;
    }
    return PreviewBitmap(bitmap.Detach(), static_cast<int>(width), static_cast<int>(height));
}

std::optional<PreviewBitmap> CreateBitmapFromEncoded(
    const std::vector<unsigned char> &bytes,
    UINT maximum_width,
    UINT maximum_height,
    std::stop_token stop_token
) {
    if (bytes.empty() || stop_token.stop_requested()) {
        return std::nullopt;
    }

    CComPtr<IWICImagingFactory> factory;
    if (FAILED(::CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    ))) {
        return std::nullopt;
    }

    UniqueGlobal memory(::GlobalAlloc(GMEM_MOVEABLE, bytes.size()));
    if (!memory) {
        return std::nullopt;
    }
    ScopedGlobalLock destination(memory.Get());
    if (destination.Data() == nullptr) {
        return std::nullopt;
    }
    std::memcpy(destination.Data(), bytes.data(), bytes.size());
    destination.Unlock();

    CComPtr<IStream> stream;
    if (FAILED(::CreateStreamOnHGlobal(memory.Get(), TRUE, &stream))) {
        return std::nullopt;
    }
    memory.Release();

    CComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(
        stream,
        nullptr,
        WICDecodeMetadataCacheOnLoad,
        &decoder
    ))) {
        return std::nullopt;
    }

    CComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return std::nullopt;
    }

    UINT source_width = 0;
    UINT source_height = 0;
    if (FAILED(frame->GetSize(&source_width, &source_height)) ||
        source_width == 0 || source_height == 0 ||
        static_cast<std::uint64_t>(source_width) * source_height > ClipboardRules::Limits::kMaximumImagePixels) {
        return std::nullopt;
    }

    UINT width = 0;
    UINT height = 0;
    if (!FitSize(source_width, source_height, maximum_width, maximum_height, width, height)) {
        return std::nullopt;
    }

    IWICBitmapSource *source = frame;
    CComPtr<IWICBitmapScaler> scaler;
    if (width != source_width || height != source_height) {
        if (stop_token.stop_requested() ||
            FAILED(factory->CreateBitmapScaler(&scaler)) ||
            FAILED(scaler->Initialize(
                frame,
                width,
                height,
                WICBitmapInterpolationModeFant
            ))) {
            return std::nullopt;
        }
        source = scaler;
    }

    CComPtr<IWICFormatConverter> converter;
    if (stop_token.stop_requested() ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(
            source,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom
        ))) {
        return std::nullopt;
    }

    if (FAILED(converter->GetSize(&width, &height)) || width == 0 || height == 0) {
        return std::nullopt;
    }

    void *bits = nullptr;
    CBitmap bitmap;
    bitmap.Attach(CreateOutputBitmap(width, height, &bits));
    if (bitmap.IsNull() || bits == nullptr) {
        return std::nullopt;
    }

    const std::uint64_t stride_value = static_cast<std::uint64_t>(width) * 4;
    const std::uint64_t buffer_size_value = stride_value * height;
    if (stride_value > std::numeric_limits<UINT>::max() ||
        buffer_size_value > std::numeric_limits<UINT>::max() ||
        stop_token.stop_requested()) {
        return std::nullopt;
    }
    const UINT stride = static_cast<UINT>(stride_value);
    const UINT buffer_size = static_cast<UINT>(buffer_size_value);
    if (FAILED(converter->CopyPixels(nullptr, stride, buffer_size, static_cast<BYTE *>(bits))) ||
        stop_token.stop_requested()) {
        return std::nullopt;
    }
    return PreviewBitmap(bitmap.Detach(), static_cast<int>(width), static_cast<int>(height));
}

} // namespace

PreviewBitmap::~PreviewBitmap() {
    if (handle != nullptr) {
        ::DeleteObject(handle);
    }
}

PreviewBitmap::PreviewBitmap(PreviewBitmap &&other) noexcept
    : handle(other.handle), width(other.width), height(other.height) {
    other.handle = nullptr;
    other.width = 0;
    other.height = 0;
}

PreviewBitmap &PreviewBitmap::operator=(PreviewBitmap &&other) noexcept {
    if (this != &other) {
        if (handle != nullptr) {
            ::DeleteObject(handle);
        }
        handle = other.handle;
        width = other.width;
        height = other.height;
        other.handle = nullptr;
        other.width = 0;
        other.height = 0;
    }
    return *this;
}

HBITMAP PreviewBitmap::Release() noexcept {
    HBITMAP result = handle;
    handle = nullptr;
    width = 0;
    height = 0;
    return result;
}

std::optional<PreviewBitmap> DecodePreviewBitmap(
    const ClipboardItem &item,
    UINT maximum_width,
    UINT maximum_height,
    std::stop_token stop_token
) {
    if (!item.has_image) {
        return std::nullopt;
    }
    for (const ClipboardFormatData &data : item.data) {
        if (stop_token.stop_requested()) {
            return std::nullopt;
        }
        if (ClipboardRules::IsEncodedImageName(data.name)) {
            if (auto bitmap = CreateBitmapFromEncoded(
                    data.bytes,
                    maximum_width,
                    maximum_height,
                    stop_token
                )) {
                return bitmap;
            }
        }
    }
    for (const ClipboardFormatData &data : item.data) {
        if (stop_token.stop_requested()) {
            return std::nullopt;
        }
        if (ClipboardRules::IsDib(data)) {
            if (auto bitmap = CreateBitmapFromDib(
                    data.bytes,
                    maximum_width,
                    maximum_height,
                    stop_token
                )) {
                return bitmap;
            }
        }
    }
    return std::nullopt;
}
