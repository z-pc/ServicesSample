#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <httplib.h>

class ApiRouter {
  public:
	using Handler = std::function<void(const httplib::Request&, httplib::Response&)>;

	void get(const std::string& path, Handler h);
	void post(const std::string& path, Handler h);

	void bind(httplib::Server& server) const;

  private:
	std::unordered_map<std::string, Handler> get_;
	std::unordered_map<std::string, Handler> post_;
};
