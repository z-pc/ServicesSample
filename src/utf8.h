#pragma once

#include <string>
#include <string_view>

namespace text {

/**
 * @brief Convert UTF-8 text to a wide string (UTF-16 on Windows).
 *
 * Intended for bridging APIs that require wide strings (e.g., Win32).
 */
std::wstring utf8_to_wide(std::string_view s);

} // namespace text
