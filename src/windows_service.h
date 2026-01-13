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

struct RecoveryAction {
	SC_ACTION_TYPE type = SC_ACTION_RESTART;
	DWORD delay_ms = 5000;
};

struct RecoveryOptions {
	// Reset failure count after N seconds. 0 => never reset (Windows semantics).
	DWORD reset_period_seconds = 24 * 60 * 60;

	// Also apply recovery actions for non-crash failures.
	bool apply_on_non_crash_failures = true;

	// Optional; used only if corresponding action types are configured.
	std::wstring reboot_message;
	std::wstring command;

	std::vector<RecoveryAction> actions;

	static RecoveryOptions defaults();
};

bool install_service(const Options& opt, const std::wstring& bin_path_with_args, std::wstring* error);
bool install_service(const Options& opt, const std::wstring& bin_path_with_args, const RecoveryOptions& recovery,
                     std::wstring* error);

bool uninstall_service(const std::wstring& service_name, std::wstring* error);

// Service lifecycle aware callback. The callback should return when stop_event is signaled.
// stop_event is a Windows HANDLE.
int run_service(const std::wstring& service_name, int (*run_callback)(void* context, void* stop_event), void* context);

void report_event_info(const std::wstring& service_name, const std::wstring& message);
void report_event_error(const std::wstring& service_name, const std::wstring& message);

} // namespace winservice

#endif
