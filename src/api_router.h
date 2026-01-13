#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <httplib.h>

class ApiRouter {
  public:
	using Handler = std::function<void(const httplib::Request&, httplib::Response&)>;
	using Next = std::function<void()>;
	using Middleware = std::function<void(const httplib::Request&, httplib::Response&, Next)>;

	void get(const std::string& path, Handler h);
	void post(const std::string& path, Handler h);

	void put(const std::string& path, Handler h);
	void del(const std::string& path, Handler h);

	void use(Middleware m);       // runs before route handler
	void use_after(Middleware m); // runs after route handler

	void group(std::string prefix, const std::function<void(ApiRouter&)>& define_routes);

	void set_not_found(Handler h);
	void
	set_exception_handler(std::function<void(const httplib::Request&, httplib::Response&, const std::exception&)> h);

	void bind(httplib::Server& server) const;

  private:
	enum class Method {
		Get,
		Post,
		Put,
		Del,
	};

	static std::string join_path(std::string_view prefix, std::string_view path);
	static std::string to_string(Method m);

	void add_route(Method method, const std::string& path, Handler h);

	Handler compose(Handler endpoint) const;

	std::unordered_map<std::string, Handler> get_;
	std::unordered_map<std::string, Handler> post_;
	std::unordered_map<std::string, Handler> put_;
	std::unordered_map<std::string, Handler> del_;

	std::vector<Middleware> before_;
	std::vector<Middleware> after_;

	std::string prefix_;

	Handler not_found_;
	std::function<void(const httplib::Request&, httplib::Response&, const std::exception&)> exception_handler_;
};
