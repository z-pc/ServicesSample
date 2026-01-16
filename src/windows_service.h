#pragma once

#include <string>
#include <vector>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace winservice {

struct Options {
	std::wstring service_name;
	std::wstring display_name;
	std::wstring description;
};

// Service lifecycle aware callback. The callback should return when stop_event is signaled.
// stop_event is a Windows HANDLE.
int run_service(const std::wstring& service_name, int (*run_callback)(void* context, void* stop_event), void* context);
void report_event_info(const std::wstring& service_name, const std::wstring& message);
void report_event_error(const std::wstring& service_name, const std::wstring& message);

} // namespace winservice

#endif
