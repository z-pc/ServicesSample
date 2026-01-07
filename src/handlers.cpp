#include "handlers.h"

#include "handlers/sample.h"
#include "api_router.h"

#include <chrono>
#include <string>

#include <nlohmann/json.hpp>

static void register_health(ApiRouter& api) {
	api.get("/health", [](const httplib::Request&, httplib::Response& res) {
		nlohmann::json j;
		j["status"] = "ok";
		res.set_content(j.dump(), "application/json");
	});
}

void register_handlers(ApiRouter& api) {
	api.get("/sample", handle_sample);
}
