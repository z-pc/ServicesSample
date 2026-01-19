#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <httplib.h>

/**
 * @brief Minimal routing layer on top of `httplib::Server`.
 *
 * Supports:
 * - Registering handlers per HTTP method + path
 * - Grouping routes under a prefix
 * - Middleware executed before and after the route handler
 * - Custom 404 handler and a global exception handler
 *
 * `bind()` wires all registered routes into an `httplib::Server`.
 */
class ApiRouter {
  public:
	/**
	 * @brief Endpoint handler.
	 *
	 * Receives the incoming request and a mutable response to fill.
	 */
	using Handler = std::function<void(const httplib::Request&, httplib::Response&)>;

	/**
	 * @brief Continuation used by middleware to proceed to the next step.
	 */
	using Next = std::function<void()>;

	/**
	 * @brief Middleware invoked with (req, res, next).
	 *
	 * Middleware is responsible for calling `next()` to continue the chain.
	 * If `next()` is not called, the request handling stops at that point.
	 */
	using Middleware = std::function<void(const httplib::Request&, httplib::Response&, Next)>;

	/**
	 * @brief Register a GET route.
	 * @throws std::logic_error if the route is duplicated.
	 */
	void get(const std::string& path, Handler h);

	/**
	 * @brief Register a POST route.
	 * @throws std::logic_error if the route is duplicated.
	 */
	void post(const std::string& path, Handler h);

	/**
	 * @brief Register a PUT route.
	 * @throws std::logic_error if the route is duplicated.
	 */
	void put(const std::string& path, Handler h);

	/**
	 * @brief Register a DELETE route.
	 * @throws std::logic_error if the route is duplicated.
	 */
	void del(const std::string& path, Handler h);

	/**
	 * @brief Add middleware executed before the route handler.
	 */
	void use(Middleware m);

	/**
	 * @brief Add middleware executed after the route handler.
	 *
	 * Note: "after" middleware runs only after the endpoint handler has completed.
	 */
	void use_after(Middleware m);

	/**
	 * @brief Define a group of routes under a prefix.
	 *
	 * The prefix is temporarily applied for the duration of `define_routes`,
	 * then restored to the previous value.
	 */
	void group(std::string prefix, const std::function<void(ApiRouter&)>& define_routes);

	/**
	 * @brief Set custom handler for HTTP 404.
	 *
	 * If not set, a plain text "Not Found" is returned.
	 */
	void set_not_found(Handler h);

	/**
	 * @brief Set global exception handler for exceptions thrown by middleware/handlers.
	 *
	 * If not set, a plain text "Internal Server Error" (500) is returned.
	 */
	void set_exception_handler(
	    std::function<void(const httplib::Request&, httplib::Response&, const std::exception&)> h);

	/**
	 * @brief Bind all registered routes and error handling into an `httplib::Server`.
	 */
	void bind(httplib::Server& server) const;

  private:
	/// @brief Internal enumeration mapping to HTTP methods.
	enum class Method {
		Get,
		Post,
		Put,
		Del,
	};

	/**
	 * @brief Join prefix and path handling slashes.
	 *
	 * Ensures there is exactly one '/' between non-empty segments.
	 */
	static std::string join_path(std::string_view prefix, std::string_view path);

	/// @brief Convert method enum to HTTP method string (e.g., "GET").
	static std::string to_string(Method m);

	/**
	 * @brief Add a route under the current prefix.
	 * @throws std::logic_error if the route is duplicated.
	 */
	void add_route(Method method, const std::string& path, Handler h);

	/**
	 * @brief Wrap an endpoint with before/after middleware and exception translation.
	 */
	Handler compose(Handler endpoint) const;

	std::unordered_map<std::string, Handler> get_;
	std::unordered_map<std::string, Handler> post_;
	std::unordered_map<std::string, Handler> put_;
	std::unordered_map<std::string, Handler> del_;

	std::vector<Middleware> before_;
	std::vector<Middleware> after_;

	/// @brief Current route group prefix (empty means no prefix).
	std::string prefix_;

	Handler not_found_;
	std::function<void(const httplib::Request&, httplib::Response&, const std::exception&)> exception_handler_;
};
