#pragma once

#include <cstddef>

#include <httplib.h>

class ApiRouter;

/**
 * @brief Thin wrapper around `httplib::Server` with basic lifecycle helpers.
 *
 * Responsibilities:
 * - Configure listening port and thread count
 * - Bind an `ApiRouter` and start the server
 * - Allow requesting stop
 */
class HttpServer {
  public:
	/**
	 * @param port TCP port to listen on.
	 * @param threads Number of worker threads (0 is allowed; interpretation depends on implementation).
	 */
	HttpServer(int port, std::size_t threads);

	/**
	 * @brief Bind the router and run the server (blocking).
	 * @return Process exit code / status code (implementation-defined).
	 */
	int run(const ApiRouter& router);

	/**
	 * @brief Request server shutdown.
	 *
	 * Safe to call from another thread.
	 */
	void stop();

  private:
	int port_ = 0;
	std::size_t threads_ = 0;

	httplib::Server server_;
};
