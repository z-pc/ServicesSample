#include "utf8.h"

#include <string>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace text {

std::wstring utf8_to_wide(std::string_view s) {
	if (s.empty()) return {};

	const int src_len = static_cast<int>(s.size());
	int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), src_len, nullptr, 0);
	if (required == 0) {
		required = MultiByteToWideChar(CP_UTF8, 0, s.data(), src_len, nullptr, 0);
		if (required == 0) return {};
	}

	std::wstring ws(static_cast<size_t>(required), L'\0');
	const int written = MultiByteToWideChar(CP_UTF8, 0, s.data(), src_len, ws.data(), required);
	if (written == 0) return {};
	return ws;
}

} // namespace text

#else

#include <codecvt>
#include <locale>

namespace text {

std::wstring utf8_to_wide(std::string_view s) {
	if (s.empty()) return {};
	try {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
		return conv.from_bytes(s.data(), s.data() + s.size());
	} catch (...) {
		return std::wstring(s.begin(), s.end());
	}
}

} // namespace text

#endif
