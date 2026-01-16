#pragma once

#include "config.h"

#include <cxxopts.hpp>

#if defined(_WIN32)
#include <string>
#endif

struct ServiceBootstrapResult {
	AppConfig config;

#if defined(_WIN32)
	std::wstring service_name;

	bool run_as_service() const { return !service_name.empty(); }
#else
	bool run_as_service() const { return false; }
#endif
};

ServiceBootstrapResult bootstrap_service(const cxxopts::ParseResult& args);
