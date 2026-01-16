#include "http_server.h"

#include "api_router.h"

#include <algorithm>
#include <thread>

#include <spdlog/spdlog.h>

HttpServer::HttpServer(int port, std::size_t threads) : port_(port), threads_(threads) {}

int HttpServer::run(const ApiRouter& router) {
	const auto hw = std::max<unsigned>(1u, std::thread::hardware_concurrency());
	const auto thread_count = threads_ == 0 ? static_cast<std::size_t>(hw) : threads_;

	// Temporarily rely on cpp-httplib's default thread pool implementation.
	server_.new_task_queue = [thread_count] { return new httplib::ThreadPool(static_cast<size_t>(thread_count)); };

	router.bind(server_);

	server_.set_error_handler([](const httplib::Request&, httplib::Response& res) {
		res.set_content("{\"error\":\"not found\"}", "application/json");
		res.status = 404;
	});

	server_.set_logger([](const httplib::Request& req, const httplib::Response& res) {
		// Observability endpoints can become hot paths; keep default access logging off for them.
		if (req.path == "/healthz" || req.path == "/metrics" || req.path == "/trace" || req.path == "/status") {
			return;
		}
		spdlog::info("{} {} {} remote_addr={}", req.method, req.path, res.status, req.remote_addr);
	});

	constexpr const char* kBindHost = "0.0.0.0";
	spdlog::info("Listening on {}:{} (threads={})", kBindHost, port_, thread_count);
	const bool ok = server_.listen(kBindHost, port_);
	return ok ? 0 : 1;
}

void HttpServer::stop() {
	server_.stop();
}
