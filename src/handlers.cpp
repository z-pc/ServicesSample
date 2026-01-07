#include "handlers.h"

#include "handlers/sample.h"
#include "api_router.h"

#include <chrono>
#include <string>

#include <nlohmann/json.hpp>

void register_handlers(ApiRouter& api) {
	api.get("/sample", handle_sample);
}
