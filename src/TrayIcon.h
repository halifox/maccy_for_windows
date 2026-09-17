#pragma once

#include "PlatformConfig.h"

#include <string_view>

HICON LoadTrayIcon(std::wstring_view name);
HICON LoadApplicationIcon();
