#include "api_router.h"
#include "config.h"
#include "handlers.h"
#include "http_server.h"
#include "service_bootstrap.h"
#include "service_defines.h"

#include <exception>
#include <string>

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

#if defined(_WIN32)
#include "windows_service.h"
#endif

int main(int argc, char** argv) {
	try {
		cxxopts::Options options(APP_SERVICE_NAME, APP_SERVICE_DESCRIPTION);
		options.add_options()("help", "Show help")("service", "Run as Windows Service (Windows only)")(
		    "service-install", "Install Windows Service (Windows only)")("service-uninstall",
		                                                                 "Uninstall Windows Service (Windows only)");

		auto result = options.parse(argc, argv);
		if (result.count("help") > 0) {
			spdlog::info("{}", options.help());
			return 0;
		}

		const std::string config_path;

		auto boot = bootstrap_service(result, config_path);
		if (boot.command_exit_code != -1) {
			return boot.command_exit_code;
		}

		AppConfig cfg = boot.config;
		ApiRouter router;
		register_handlers(router);
		HttpServer server(cfg.host, cfg.port, cfg.threads);

#if defined(_WIN32)
		if (boot.run_as_service()) {
			static HttpServer* g_server = nullptr;
			static ApiRouter* g_router = nullptr;
			g_server = &server;
			g_router = &router;
			return winservice::run_service(boot.service_name, []() -> int { return g_server->run(*g_router); });
		}
#endif

		return server.run(router);
	} catch (const std::exception& ex) {
		spdlog::error("Fatal: {}", ex.what());
		return 1;
	}
}
