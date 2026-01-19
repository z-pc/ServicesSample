#pragma once

#include "api_router.h"

namespace obs {

/**
 * @brief Options controlling built-in observability endpoints and behavior.
 */
struct ObservabilityOptions {
	bool enable_health = true;
	bool enable_metrics = true;
	bool enable_tracing = true;

	/// @brief Skip access logging for observability endpoints to reduce noise/hot-path overhead.
	bool skip_access_log_for_observability = true;

	std::string health_path = "/healthz";
	std::string metrics_path = "/metrics";
	std::string trace_path = "/trace";
	std::string status_path = "/status";
};

/**
 * @brief Register health/metrics/tracing/status endpoints and associated middleware on the router.
 */
void register_observability(ApiRouter& api, const ObservabilityOptions& opt = {});

} // namespace obs
