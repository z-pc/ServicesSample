#pragma once

#include <string>

#if defined(_WIN32)

namespace winservice {

struct Options {
	std::wstring service_name;
	std::wstring display_name;
	std::wstring description;
};

bool install_service(const Options& opt, const std::wstring& bin_path_with_args, std::wstring* error);
bool uninstall_service(const std::wstring& service_name, std::wstring* error);
int run_service(const std::wstring& service_name, int (*run_callback)(void));
void report_event_info(const std::wstring& service_name, const std::wstring& message);
void report_event_error(const std::wstring& service_name, const std::wstring& message);

} // namespace winservice

#endif
