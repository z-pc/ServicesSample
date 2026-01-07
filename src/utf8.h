#pragma once

#include <string>
#include <string_view>

namespace text {

std::wstring utf8_to_wide(std::string_view s);

} // namespace text
