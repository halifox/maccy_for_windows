#pragma once

#include "PlatformConfig.h"
#include "Win32Resources.h"

#include <shellapi.h>

#include <string_view>

HICON LoadTrayIcon(std::wstring_view name);
HICON LoadApplicationIcon();

class TrayIcon {
public:
    TrayIcon() = default;
    ~TrayIcon() { Remove(); }

    TrayIcon(const TrayIcon &) = delete;
    TrayIcon &operator=(const TrayIcon &) = delete;

    bool Add(HWND owner, UINT icon_id, UINT callback_message,
             std::wstring_view tooltip, std::wstring_view icon_name);
    bool UpdateIcon(std::wstring_view icon_name);
    bool IsAdded() const noexcept { return m_added; }
    bool GetRect(RECT &rect) const noexcept;
    void Remove() noexcept;

private:
    NOTIFYICONDATAW m_data{};
    UniqueIcon m_icon;
    bool m_added = false;
};
