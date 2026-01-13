#include "api_router.h"

#include <algorithm>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

static void respond_plain(httplib::Response& res, int status, std::string_view body) {
	res.status = status;
	res.set_content(std::string(body), "text/plain; charset=utf-8");
}

std::string ApiRouter::join_path(std::string_view prefix, std::string_view path) {
	std::string p(prefix);
	std::string s(path);

	if (p.empty()) return s;
	if (s.empty()) return p;

	const bool p_slash = !p.empty() && p.back() == '/';
	const bool s_slash = !s.empty() && s.front() == '/';

	if (p_slash && s_slash) {
		p.pop_back();
		return p + s;
	}
	if (!p_slash && !s_slash) {
		return p + "/" + s;
	}
	return p + s;
}

std::string ApiRouter::to_string(Method m) {
	switch (m) {
	case Method::Get:
		return "GET";
	case Method::Post:
		return "POST";
	case Method::Put:
		return "PUT";
	case Method::Del:
		return "DELETE";
	default:
		return "UNKNOWN";
	}
}

void ApiRouter::add_route(Method method, const std::string& path, Handler h) {
	const std::string full_path = join_path(prefix_, path);

	auto insert_or_throw = [&](auto& map) {
		auto [it, inserted] = map.emplace(full_path, std::move(h));
		if (!inserted) {
			throw std::logic_error("ApiRouter duplicate route: " + to_string(method) + " " + full_path);
		}
	};

	switch (method) {
	case Method::Get:
		insert_or_throw(get_);
		break;
	case Method::Post:
		insert_or_throw(post_);
		break;
	case Method::Put:
		insert_or_throw(put_);
		break;
	case Method::Del:
		insert_or_throw(del_);
		break;
	}
}

void ApiRouter::get(const std::string& path, Handler h) {
	add_route(Method::Get, path, std::move(h));
}

void ApiRouter::post(const std::string& path, Handler h) {
	add_route(Method::Post, path, std::move(h));
}

void ApiRouter::put(const std::string& path, Handler h) {
	add_route(Method::Put, path, std::move(h));
}

void ApiRouter::del(const std::string& path, Handler h) {
	add_route(Method::Del, path, std::move(h));
}

void ApiRouter::use(Middleware m) {
	before_.push_back(std::move(m));
}

void ApiRouter::use_after(Middleware m) {
	after_.push_back(std::move(m));
}

void ApiRouter::group(std::string prefix, const std::function<void(ApiRouter&)>& define_routes) {
	const std::string prev = prefix_;
	prefix_ = join_path(prefix_, prefix);
	define_routes(*this);
	prefix_ = prev;
}

void ApiRouter::set_not_found(Handler h) {
	not_found_ = std::move(h);
}

void ApiRouter::set_exception_handler(
    std::function<void(const httplib::Request&, httplib::Response&, const std::exception&)> h) {
	exception_handler_ = std::move(h);
}

ApiRouter::Handler ApiRouter::compose(Handler endpoint) const {
	return [this, endpoint = std::move(endpoint)](const httplib::Request& req, httplib::Response& res) {
		auto invoke_chain = [&](const std::vector<Middleware>& chain, Next final_next) {
			size_t i = 0;
			Next next = [&]() {
				if (i >= chain.size()) {
					final_next();
					return;
				}
				const auto& m = chain[i++];
				m(req, res, next);
			};
			next();
		};

		try {
			invoke_chain(before_, [&]() {
				endpoint(req, res);
				invoke_chain(after_, []() {});
			});
		} catch (const std::exception& ex) {
			if (exception_handler_) {
				exception_handler_(req, res, ex);
				return;
			}
			respond_plain(res, 500, "Internal Server Error");
		}
	};
}

void ApiRouter::bind(httplib::Server& server) const {
	for (const auto& kv : get_) {
		server.Get(kv.first, compose(kv.second));
	}
	for (const auto& kv : post_) {
		server.Post(kv.first, compose(kv.second));
	}
	for (const auto& kv : put_) {
		server.Put(kv.first, compose(kv.second));
	}
	for (const auto& kv : del_) {
		server.Delete(kv.first, compose(kv.second));
	}

	server.set_error_handler([this](const httplib::Request& req, httplib::Response& res) {
		if (res.status == 404 && not_found_) {
			compose(not_found_)(req, res);
			return;
		}

		if (res.status == 404) {
			respond_plain(res, 404, "Not Found");
			return;
		}
		if (res.status == 405) {
			respond_plain(res, 405, "Method Not Allowed");
			return;
		}
	});
}
