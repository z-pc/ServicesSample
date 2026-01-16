#pragma once

#include <cstddef>

#include <httplib.h>

class ApiRouter;

class HttpServer {
  public:
	HttpServer(int port, std::size_t threads);
	int run(const ApiRouter& router);

	void stop();

  private:
	int port_ = 0;
	std::size_t threads_ = 0;

	httplib::Server server_;
};
