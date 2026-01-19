#pragma once

#include "config.h"

#include <cxxopts.hpp>

#if defined(_WIN32)
#include <string>
#endif

/**
 * @brief Output of application/service bootstrap.
 *
 * Contains resolved configuration and (on Windows) the service name indicating
 * whether the process should run under the Service Control Manager (SCM).
 */
struct ServiceBootstrapResult {
	/// @brief Application configuration derived from CLI + config sources.
	AppConfig config;

#if defined(_WIN32)
	/// @brief Non-empty => run as Windows service under this name.
	std::wstring service_name;

	/// @brief True when `service_name` is provided.
	bool run_as_service() const { return !service_name.empty(); }
#else
	/// @brief Non-Windows builds don't support SCM-mode.
	bool run_as_service() const { return false; }
#endif
};

/**
 * @brief Perform startup/bootstrap logic from parsed CLI arguments.
 *
 * Typical responsibilities include:
 * - Load config file (if any)
 * - Apply CLI overrides
 * - Resolve platform defaults
 * - Determine service mode (Windows)
 */
ServiceBootstrapResult bootstrap_service(const cxxopts::ParseResult& args);
