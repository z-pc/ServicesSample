#include "observability.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <random>
#include <sstream>
#include <string>

namespace {

using clock_t = std::chrono::steady_clock;

struct Metrics {
	std::atomic<uint64_t> requests_total{0};
	std::atomic<uint64_t> requests_in_flight{0};

	// Coarse latency buckets (in ms) to keep overhead very low.
	std::atomic<uint64_t> latency_le_1ms{0};
	std::atomic<uint64_t> latency_le_5ms{0};
	std::atomic<uint64_t> latency_le_10ms{0};
	std::atomic<uint64_t> latency_le_25ms{0};
	std::atomic<uint64_t> latency_le_50ms{0};
	std::atomic<uint64_t> latency_le_100ms{0};
	std::atomic<uint64_t> latency_le_250ms{0};
	std::atomic<uint64_t> latency_le_500ms{0};
	std::atomic<uint64_t> latency_le_1000ms{0};
	std::atomic<uint64_t> latency_gt_1000ms{0};
};

Metrics& metrics() {
	static Metrics m;
	return m;
}

static bool is_observability_path(const std::string& path, const obs::ObservabilityOptions& opt) {
	return path == opt.health_path || path == opt.metrics_path || path == opt.trace_path || path == opt.status_path;
}

static void observe_latency(std::chrono::milliseconds ms) {
	auto& m = metrics();
	const auto v = ms.count();
	if (v <= 1)
		m.latency_le_1ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 5)
		m.latency_le_5ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 10)
		m.latency_le_10ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 25)
		m.latency_le_25ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 50)
		m.latency_le_50ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 100)
		m.latency_le_100ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 250)
		m.latency_le_250ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 500)
		m.latency_le_500ms.fetch_add(1, std::memory_order_relaxed);
	else if (v <= 1000)
		m.latency_le_1000ms.fetch_add(1, std::memory_order_relaxed);
	else
		m.latency_gt_1000ms.fetch_add(1, std::memory_order_relaxed);
}

static std::string gen_trace_id_32hex() {
	static thread_local std::mt19937_64 rng{std::random_device{}()};
	std::ostringstream oss;
	oss << std::hex;
	for (int i = 0; i < 4; ++i) {
		const uint64_t x = rng();
		oss.width(16);
		oss.fill('0');
		oss << x;
	}
	std::string s = oss.str();
	if (s.size() > 32) s.resize(32);
	if (s.size() < 32) s.append(32 - s.size(), '0');
	return s;
}

static std::string gen_span_id_16hex() {
	static thread_local std::mt19937_64 rng{std::random_device{}()};
	std::ostringstream oss;
	oss << std::hex;
	const uint64_t x = rng();
	oss.width(16);
	oss.fill('0');
	oss << x;
	return oss.str();
}

static std::string get_or_make_request_id(const httplib::Request& req) {
	// Best-effort correlation id: prefer inbound, else generate.
	auto it = req.headers.find("x-request-id");
	if (it == req.headers.end()) it = req.headers.find("X-Request-Id");
	if (it != req.headers.end() && !it->second.empty()) return it->second;
	return gen_span_id_16hex();
}

static std::string get_or_make_traceparent(const httplib::Request& req, std::string& out_trace_id) {
	// Minimal W3C traceparent handling: accept well-formed `traceparent` or generate.
	auto it = req.headers.find("traceparent");
	if (it == req.headers.end()) it = req.headers.find("Traceparent");
	if (it != req.headers.end()) {
		const std::string& v = it->second;
		// Expected: "00-<32hex>-<16hex>-<2hex>".
		if (v.size() >= 55 && v[2] == '-' && v[35] == '-' && v[52] == '-') {
			out_trace_id = v.substr(3, 32);
			return v;
		}
	}
	out_trace_id = gen_trace_id_32hex();
	const std::string span_id = gen_span_id_16hex();
	return "00-" + out_trace_id + "-" + span_id + "-01";
}

} // namespace

void obs::register_observability(ApiRouter& api, const ObservabilityOptions& opt) {
	// Before middleware: request counters, in-flight, and start time.
	api.use([opt](const httplib::Request& req, httplib::Response& res, ApiRouter::Next next) {
		if (opt.enable_tracing) {
			std::string trace_id;
			const std::string tp = get_or_make_traceparent(req, trace_id);
			res.set_header("traceparent", tp);
			res.set_header("x-request-id", get_or_make_request_id(req));
		} else {
			res.set_header("x-request-id", get_or_make_request_id(req));
		}

		auto& m = metrics();
		m.requests_total.fetch_add(1, std::memory_order_relaxed);
		m.requests_in_flight.fetch_add(1, std::memory_order_relaxed);

		const auto start = clock_t::now();
		next();
		const auto end = clock_t::now();

		m.requests_in_flight.fetch_sub(1, std::memory_order_relaxed);
		observe_latency(std::chrono::duration_cast<std::chrono::milliseconds>(end - start));
	});

	if (opt.enable_health) {
		api.get(opt.health_path, [](const httplib::Request&, httplib::Response& res) {
			res.status = 200;
			res.set_content("ok", "text/plain; charset=utf-8");
		});
	}

	if (opt.enable_metrics) {
		api.get(opt.metrics_path, [](const httplib::Request&, httplib::Response& res) {
			const auto& m = metrics();
			// Prometheus text format.
			std::ostringstream out;
			out << "# HELP http_requests_total Total HTTP requests received\n";
			out << "# TYPE http_requests_total counter\n";
			out << "http_requests_total " << m.requests_total.load(std::memory_order_relaxed) << "\n";

			out << "# HELP http_requests_in_flight In-flight HTTP requests\n";
			out << "# TYPE http_requests_in_flight gauge\n";
			out << "http_requests_in_flight " << m.requests_in_flight.load(std::memory_order_relaxed) << "\n";

			out << "# HELP http_request_duration_ms_bucket Coarse request duration histogram buckets\n";
			out << "# TYPE http_request_duration_ms_bucket counter\n";
			out << "http_request_duration_ms_bucket{le=\"1\"} " << m.latency_le_1ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"5\"} " << m.latency_le_5ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"10\"} " << m.latency_le_10ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"25\"} " << m.latency_le_25ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"50\"} " << m.latency_le_50ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"100\"} " << m.latency_le_100ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"250\"} " << m.latency_le_250ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"500\"} " << m.latency_le_500ms.load(std::memory_order_relaxed)
			    << "\n";
			out << "http_request_duration_ms_bucket{le=\"1000\"} "
			    << m.latency_le_1000ms.load(std::memory_order_relaxed) << "\n";
			out << "http_request_duration_ms_bucket{le=\"+Inf\"} "
			    << (m.latency_le_1ms.load(std::memory_order_relaxed) +
			        m.latency_le_5ms.load(std::memory_order_relaxed) +
			        m.latency_le_10ms.load(std::memory_order_relaxed) +
			        m.latency_le_25ms.load(std::memory_order_relaxed) +
			        m.latency_le_50ms.load(std::memory_order_relaxed) +
			        m.latency_le_100ms.load(std::memory_order_relaxed) +
			        m.latency_le_250ms.load(std::memory_order_relaxed) +
			        m.latency_le_500ms.load(std::memory_order_relaxed) +
			        m.latency_le_1000ms.load(std::memory_order_relaxed) +
			        m.latency_gt_1000ms.load(std::memory_order_relaxed))
			    << "\n";

			res.status = 200;
			res.set_content(out.str(), "text/plain; version=0.0.4; charset=utf-8");
		});
	}

	if (opt.enable_tracing) {
		api.get(opt.trace_path, [](const httplib::Request& req, httplib::Response& res) {
			std::string trace_id;
			const std::string tp = get_or_make_traceparent(req, trace_id);
			const std::string rid = get_or_make_request_id(req);
			res.status = 200;
			res.set_header("traceparent", tp);
			res.set_header("x-request-id", rid);
			res.set_content("traceparent=" + tp + "\n" + "x-request-id=" + rid + "\n", "text/plain; charset=utf-8");
		});
	}

	// Convenience endpoint to return a compact JSON-like status.
	api.get(opt.status_path, [](const httplib::Request&, httplib::Response& res) {
		auto& m = metrics();
		std::ostringstream out;
		out << "{\"requests_total\":" << m.requests_total.load(std::memory_order_relaxed) << ",";
		out << "\"requests_in_flight\":" << m.requests_in_flight.load(std::memory_order_relaxed) << "}";
		res.status = 200;
		res.set_content(out.str(), "application/json");
	});
}
