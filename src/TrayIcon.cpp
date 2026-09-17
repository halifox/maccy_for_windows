#include "TrayIcon.h"

#include "resource.h"

#include <atlbase.h>
#include <wincodec.h>
#include <winreg.h>

#include <string_view>

namespace {

struct TrayIconResources {
    int light_16;
    int dark_16;
    int light_32;
    int dark_32;
};

constexpr TrayIconResources kMaccyIcons{
    IDR_TRAY_MACCY_LIGHT_16,
    IDR_TRAY_MACCY_DARK_16,
    IDR_TRAY_MACCY_LIGHT_32,
    IDR_TRAY_MACCY_DARK_32,
};
constexpr TrayIconResources kClipboardIcons{
    IDR_TRAY_CLIPBOARD_LIGHT_16,
    IDR_TRAY_CLIPBOARD_DARK_16,
    IDR_TRAY_CLIPBOARD_LIGHT_32,
    IDR_TRAY_CLIPBOARD_DARK_32,
};
constexpr TrayIconResources kScissorsIcons{
    IDR_TRAY_SCISSORS_LIGHT_16,
    IDR_TRAY_SCISSORS_DARK_16,
    IDR_TRAY_SCISSORS_LIGHT_32,
    IDR_TRAY_SCISSORS_DARK_32,
};
constexpr TrayIconResources kPaperclipIcons{
    IDR_TRAY_PAPERCLIP_LIGHT_16,
    IDR_TRAY_PAPERCLIP_DARK_16,
    IDR_TRAY_PAPERCLIP_LIGHT_32,
    IDR_TRAY_PAPERCLIP_DARK_32,
};

const TrayIconResources& ResourcesForName(std::wstring_view name) noexcept {
    if (name == L"clipboard") {
        return kClipboardIcons;
    }
    if (name == L"scissors") {
        return kScissorsIcons;
    }
    if (name == L"paperclip") {
        return kPaperclipIcons;
    }
    return kMaccyIcons;
}

bool IsDarkSystemTheme() noexcept {
    HKEY key = nullptr;
    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            0,
            KEY_QUERY_VALUE,
            &key
        ) != ERROR_SUCCESS) {
        return false;
    }

    DWORD value = 1;
    DWORD value_size = sizeof(value);
    DWORD value_type = 0;
    const LSTATUS result = RegQueryValueExW(
        key,
        L"SystemUsesLightTheme",
        nullptr,
        &value_type,
        reinterpret_cast<LPBYTE>(&value),
        &value_size
    );
    RegCloseKey(key);
    return result == ERROR_SUCCESS && value_type == REG_DWORD && value == 0;
}

HICON CreateIconFromPngResource(int resource_id) noexcept {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(resource_id), RT_RCDATA);
    if (resource == nullptr) {
        return nullptr;
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    const DWORD resource_size = SizeofResource(instance, resource);
    if (loaded == nullptr || resource_size == 0) {
        return nullptr;
    }

    HGLOBAL stream_memory = GlobalAlloc(GMEM_MOVEABLE, resource_size);
    if (stream_memory == nullptr) {
        return nullptr;
    }
    void* destination = GlobalLock(stream_memory);
    if (destination == nullptr) {
        GlobalFree(stream_memory);
        return nullptr;
    }
    CopyMemory(destination, LockResource(loaded), resource_size);
    GlobalUnlock(stream_memory);

    CComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(stream_memory, TRUE, &stream))) {
        GlobalFree(stream_memory);
        return nullptr;
    }

    CComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory)
        ))) {
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

    UINT width = 0;
    UINT height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0 ||
        width > 256 || height > 256) {
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

    const UINT stride = width * 4;
    const UINT buffer_size = stride * height;
    BITMAPINFO bitmap_info{};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = static_cast<LONG>(width);
    bitmap_info.bmiHeader.biHeight = -static_cast<LONG>(height);
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    HBITMAP color_bitmap = CreateDIBSection(
        nullptr,
        &bitmap_info,
        DIB_RGB_COLORS,
        &pixels,
        nullptr,
        0
    );
    if (color_bitmap == nullptr || pixels == nullptr ||
        FAILED(converter->CopyPixels(nullptr, stride, buffer_size, static_cast<BYTE *>(pixels)))) {
        if (color_bitmap != nullptr) {
            DeleteObject(color_bitmap);
        }
        return nullptr;
    }

    HBITMAP mask_bitmap = CreateBitmap(
        static_cast<int>(width),
        static_cast<int>(height),
        1,
        1,
        nullptr
    );
    if (mask_bitmap == nullptr) {
        DeleteObject(color_bitmap);
        return nullptr;
    }

    ICONINFO icon_info{};
    icon_info.fIcon = TRUE;
    icon_info.hbmColor = color_bitmap;
    icon_info.hbmMask = mask_bitmap;
    HICON icon = CreateIconIndirect(&icon_info);
    DeleteObject(mask_bitmap);
    DeleteObject(color_bitmap);
    return icon;
}

HICON CreateFallbackIcon() noexcept {
    const HICON application_icon = LoadIconW(nullptr, IDI_APPLICATION);
    return application_icon == nullptr ? nullptr : CopyIcon(application_icon);
}

} // namespace

HICON LoadApplicationIcon() {
    HICON icon = CreateIconFromPngResource(IDR_APP_MACCY_ICON);
    return icon != nullptr ? icon : CreateFallbackIcon();
}

HICON LoadTrayIcon(std::wstring_view name) {
    const TrayIconResources& resources = ResourcesForName(name);
    const bool use_large_icon = GetSystemMetrics(SM_CXSMICON) > 16;
    const bool dark_theme = IsDarkSystemTheme();
    const int resource_id = dark_theme
        ? (use_large_icon ? resources.dark_32 : resources.dark_16)
        : (use_large_icon ? resources.light_32 : resources.light_16);
    HICON icon = CreateIconFromPngResource(resource_id);
    return icon != nullptr ? icon : CreateFallbackIcon();
}
