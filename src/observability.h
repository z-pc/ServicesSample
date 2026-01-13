#pragma once

#include "api_router.h"

namespace obs {

struct ObservabilityOptions {
	bool enable_health = true;
	bool enable_metrics = true;
	bool enable_tracing = true;

	// Skip logging for endpoints that can become hot paths.
	bool skip_access_log_for_observability = true;

	std::string health_path = "/healthz";
	std::string metrics_path = "/metrics";
	std::string trace_path = "/trace";
	std::string status_path = "/status";
};

void register_observability(ApiRouter& api, const ObservabilityOptions& opt = {});

} // namespace obs
