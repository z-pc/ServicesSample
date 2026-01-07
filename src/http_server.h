#pragma once

#include <cstddef>
#include <string>

class ApiRouter;

class HttpServer {
  public:
	HttpServer(std::string host, int port, std::size_t threads);
	int run(const ApiRouter& router);

  private:
	std::string host_;
	int port_ = 0;
	std::size_t threads_ = 0;
};
