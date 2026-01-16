#include "handlers.h"

#include "api_router.h"
#include "observability.h"

#include <chrono>
#include <string>

#include <nlohmann/json.hpp>

void register_handlers(ApiRouter& api) {
	obs::ObservabilityOptions opt;
	opt.enable_metrics = false;
	opt.enable_tracing = false;
	obs::register_observability(api, opt);

	api.get("/hello", [](const httplib::Request& req, httplib::Response& res) {
		nlohmann::json j;
		j["message"] = "Hello, World!";
		res.status = 200;
		res.set_content(j.dump(), "application/json");
	});
}
