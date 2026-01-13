#include "http_server.h"

#include "api_router.h"

#include <algorithm>
#include <thread>

#include <spdlog/spdlog.h>

HttpServer::HttpServer(std::string host, int port, std::size_t threads)
    : host_(std::move(host)), port_(port), threads_(threads) {}

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
		spdlog::info("{} {} {} remote_addr={}", req.method, req.path, res.status, req.remote_addr);
	});

	spdlog::info("Listening on {}:{} (threads={})", host_, port_, thread_count);
	const bool ok = server_.listen(host_, port_);
	return ok ? 0 : 1;
}

void HttpServer::stop() {
	server_.stop();
}
