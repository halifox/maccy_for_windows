#include "TrayIcon.h"

#include "resource.h"

#include <atlbase.h>
#include <atlapp.h>
#include <atlgdi.h>
#include <wincodec.h>
#include <winreg.h>

#include <algorithm>
#include <string_view>
#include <utility>

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

UniqueIcon CreateIconFromPngResource(int resource_id) noexcept {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    const HRSRC resource = FindResourceW(instance, MAKEINTRESOURCEW(resource_id), RT_RCDATA);
    if (resource == nullptr) {
        return {};
    }

    const HGLOBAL loaded = LoadResource(instance, resource);
    const DWORD resource_size = SizeofResource(instance, resource);
    if (loaded == nullptr || resource_size == 0) {
        return {};
    }

    UniqueGlobal stream_memory(GlobalAlloc(GMEM_MOVEABLE, resource_size));
    if (!stream_memory) {
        return {};
    }
    ScopedGlobalLock destination(stream_memory.Get());
    if (destination.Data() == nullptr) {
        return {};
    }
    CopyMemory(destination.Data(), LockResource(loaded), resource_size);
    destination.Unlock();

    CComPtr<IStream> stream;
    if (FAILED(CreateStreamOnHGlobal(stream_memory.Get(), TRUE, &stream))) {
        return {};
    }
    stream_memory.Release();

    CComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&factory)
        ))) {
        return {};
    }

    CComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromStream(
            stream,
            nullptr,
            WICDecodeMetadataCacheOnLoad,
            &decoder
        ))) {
        return {};
    }

    CComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, &frame))) {
        return {};
    }

    UINT width = 0;
    UINT height = 0;
    if (FAILED(frame->GetSize(&width, &height)) || width == 0 || height == 0 ||
        width > 256 || height > 256) {
        return {};
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
        return {};
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
    CBitmap color_bitmap;
    color_bitmap.Attach(CreateDIBSection(
        nullptr,
        &bitmap_info,
        DIB_RGB_COLORS,
        &pixels,
        nullptr,
        0
    ));
    if (color_bitmap.IsNull() || pixels == nullptr ||
        FAILED(converter->CopyPixels(nullptr, stride, buffer_size, static_cast<BYTE *>(pixels)))) {
        return {};
    }

    CBitmap mask_bitmap;
    mask_bitmap.Attach(CreateBitmap(
        static_cast<int>(width),
        static_cast<int>(height),
        1,
        1,
        nullptr
    ));
    if (mask_bitmap.IsNull()) {
        return {};
    }

    ICONINFO icon_info{};
    icon_info.fIcon = TRUE;
    icon_info.hbmColor = color_bitmap;
    icon_info.hbmMask = mask_bitmap;
    return UniqueIcon(CreateIconIndirect(&icon_info));
}

UniqueIcon CreateFallbackIcon() noexcept {
    const HICON application_icon = LoadIconW(nullptr, IDI_APPLICATION);
    return UniqueIcon(application_icon == nullptr ? nullptr : CopyIcon(application_icon));
}

} // namespace

UniqueIcon LoadApplicationIcon() {
    UniqueIcon icon = CreateIconFromPngResource(IDR_APP_MACCY_PNG);
    if (icon) {
        return icon;
    }
    return CreateFallbackIcon();
}

UniqueIcon LoadTrayIcon(std::wstring_view name) {
    const TrayIconResources& resources = ResourcesForName(name);
    const bool use_large_icon = GetSystemMetrics(SM_CXSMICON) > 16;
    const bool dark_theme = IsDarkSystemTheme();
    const int resource_id = dark_theme
        ? (use_large_icon ? resources.dark_32 : resources.dark_16)
        : (use_large_icon ? resources.light_32 : resources.light_16);
    UniqueIcon icon = CreateIconFromPngResource(resource_id);
    if (icon) {
        return icon;
    }
    return CreateFallbackIcon();
}

bool TrayIcon::Add(HWND owner, UINT icon_id, UINT callback_message,
                   std::wstring_view tooltip, std::wstring_view icon_name) {
    Remove();

    UniqueIcon icon = LoadTrayIcon(icon_name);
    if (!icon) {
        return false;
    }

    NOTIFYICONDATAW data{};
    data.cbSize = sizeof(data);
    data.hWnd = owner;
    data.uID = icon_id;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = callback_message;
    data.hIcon = icon.Get();
    const auto tooltip_length = std::min(tooltip.size(), ARRAYSIZE(data.szTip) - 1);
    if (tooltip_length != 0) {
        std::copy_n(tooltip.data(), tooltip_length, data.szTip);
    }
    data.szTip[tooltip_length] = L'\0';
    if (!::Shell_NotifyIconW(NIM_ADD, &data)) {
        return false;
    }

    m_data = data;
    m_icon = std::move(icon);
    m_added = true;
    return true;
}

bool TrayIcon::UpdateIcon(std::wstring_view icon_name) {
    if (!m_added) {
        return false;
    }

    UniqueIcon icon = LoadTrayIcon(icon_name);
    if (!icon) {
        return false;
    }

    NOTIFYICONDATAW data = m_data;
    data.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    data.hIcon = icon.Get();
    if (!::Shell_NotifyIconW(NIM_MODIFY, &data)) {
        return false;
    }

    m_data = data;
    m_icon = std::move(icon);
    return true;
}

bool TrayIcon::GetRect(RECT &rect) const noexcept {
    if (!m_added) {
        return false;
    }

    NOTIFYICONIDENTIFIER identifier{};
    identifier.cbSize = sizeof(identifier);
    identifier.hWnd = m_data.hWnd;
    identifier.uID = m_data.uID;
    return SUCCEEDED(::Shell_NotifyIconGetRect(&identifier, &rect));
}

void TrayIcon::Remove() noexcept {
    if (m_added) {
        ::Shell_NotifyIconW(NIM_DELETE, &m_data);
        m_added = false;
    }
    m_icon.Reset();
    m_data = {};
}
