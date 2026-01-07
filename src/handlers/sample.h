#pragma once

#include "httplib.h"
#include "nlohmann/json.hpp"

class ApiRouter;

void handle_sample(const httplib::Request& req, httplib::Response& res) {
	nlohmann::json j;
	j["status"] = "ok";
	res.set_content(j.dump(), "application/json");
}
