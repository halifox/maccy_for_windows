#pragma once

#include "PlatformConfig.h"
#include "resource.h"

#include <initializer_list>
#include <string>
#include <string_view>

namespace Localization {

void Initialize() noexcept;
bool IsChinese() noexcept;

std::wstring Text(UINT resource_id);
std::wstring FromUtf8(std::string_view value);
std::wstring Format(
    UINT resource_id,
    std::initializer_list<std::wstring_view> values
);

} // namespace Localization
