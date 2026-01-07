#include "api_router.h"

void ApiRouter::get(const std::string& path, Handler h) {
	get_[path] = std::move(h);
}

void ApiRouter::post(const std::string& path, Handler h) {
	post_[path] = std::move(h);
}

void ApiRouter::bind(httplib::Server& server) const {
	for (const auto& kv : get_) {
		server.Get(kv.first, kv.second);
	}
	for (const auto& kv : post_) {
		server.Post(kv.first, kv.second);
	}
}
