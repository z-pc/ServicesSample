#pragma once

#include "config.h"

#include <cxxopts.hpp>

#if defined(_WIN32)
#include <string>
#endif

struct ServiceBootstrapResult {
	AppConfig config;

#if defined(_WIN32)
	// -1 => not handled, otherwise return code to exit process
	int command_exit_code = -1;
	std::wstring service_name;

	bool run_as_service() const { return !service_name.empty() && command_exit_code == -1; }
#else
	int command_exit_code = -1;
	bool run_as_service() const { return false; }
#endif
};

ServiceBootstrapResult bootstrap_service(const cxxopts::ParseResult& args, const std::string& config_path);
