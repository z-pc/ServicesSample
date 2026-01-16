#include "api_router.h"
#include "config.h"
#include "handlers.h"
#include "http_server.h"
#include "service_bootstrap.h"
#include "service_defines.h"

#include <exception>
#include <string>
#include <thread>

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>

#if defined(_WIN32)
#include <windows.h>

#include "windows_service.h"
#endif

int main(int argc, char** argv) {
	try {
		cxxopts::Options options(APP_SERVICE_NAME, APP_SERVICE_DESCRIPTION);
		options.add_options()("help", "Show help")("service", "Run as Windows Service (Windows only)");

		auto result = options.parse(argc, argv);
		if (result.count("help") > 0) {
			spdlog::info("{}", options.help());
			return 0;
		}

		auto boot = bootstrap_service(result);

		AppConfig cfg = boot.config;
		ApiRouter router;
		register_handlers(router);
		HttpServer server(cfg.port, cfg.threads);

#if defined(_WIN32)
		if (boot.run_as_service()) {
			struct Ctx {
				HttpServer* server;
				ApiRouter* router;
			} ctx{&server, &router};

			auto run = [](void* ctxp, void* stop_evt) -> int {
				auto* ctx = static_cast<Ctx*>(ctxp);
				HANDLE ev = static_cast<HANDLE>(stop_evt);

				std::thread stopper([ctx, ev]() {
					WaitForSingleObject(ev, INFINITE);
					ctx->server->stop();
				});

				const int rc = ctx->server->run(*ctx->router);
				if (stopper.joinable()) stopper.join();
				return rc;
			};

			return winservice::run_service(boot.service_name, run, &ctx);
		}
#endif

		return server.run(router);
	} catch (const std::exception& ex) {
		spdlog::error("Fatal: {}", ex.what());
		return 1;
	}
}
