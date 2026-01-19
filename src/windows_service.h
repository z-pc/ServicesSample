#pragma once

#include <string>
#include <vector>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace winservice {

/**
 * @brief Metadata used for Windows service install/config flows (if implemented).
 */
struct Options {
	std::wstring service_name;
	std::wstring display_name;
	std::wstring description;
};

/**
 * @brief Run as a Windows Service controlled by SCM.
 *
 * The callback must block until `stop_event` is signaled, then return.
 *
 * @param service_name Name registered with SCM.
 * @param run_callback Function invoked by the service runtime.
 * @param context Opaque user context forwarded to the callback.
 * @return Exit/status code (implementation-defined).
 */
int run_service(const std::wstring& service_name, int (*run_callback)(void* context, void* stop_event), void* context);

/**
 * @brief Write an informational entry to the Windows Event Log for the given service.
 */
void report_event_info(const std::wstring& service_name, const std::wstring& message);

/**
 * @brief Write an error entry to the Windows Event Log for the given service.
 */
void report_event_error(const std::wstring& service_name, const std::wstring& message);

} // namespace winservice

#endif
