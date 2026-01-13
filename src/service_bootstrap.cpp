#include "service_bootstrap.h"

#include "service_defines.h"

#include <filesystem>
#include <system_error>
#include <vector>

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#if defined(_WIN32)
#include "windows_service.h"
#include "utf8.h"
#endif

namespace {

static std::filesystem::path exe_dir() {
#if defined(_WIN32)
	wchar_t exe_path[MAX_PATH]{};
	const DWORD n = GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
	if (n == 0) return std::filesystem::current_path();
	return std::filesystem::path(exe_path).parent_path();
#else
	return std::filesystem::current_path();
#endif
}

static std::filesystem::path default_data_dir() {
#if defined(_WIN32)
	const char* program_data = std::getenv("ProgramData");
	if (program_data && *program_data) {
		return std::filesystem::path(program_data) / APP_SERVICE_NAME;
	}
	return std::filesystem::path("C:\\ProgramData") / APP_SERVICE_NAME;
#else
	return std::filesystem::current_path() / "data";
#endif
}

static void init_logging(const std::filesystem::path& logs_dir, bool is_service_mode
#if defined(_WIN32)
                         ,
                         const std::wstring& service_name
#endif
) {
	std::error_code ec;
	std::filesystem::create_directories(logs_dir, ec);
	if (ec) {
#if defined(_WIN32)
		if (is_service_mode && !service_name.empty()) {
			winservice::report_event_error(service_name, L"Failed to create logs directory: " +
			                                                 text::utf8_to_wide(logs_dir.string()) + L" err=" +
			                                                 text::utf8_to_wide(ec.message()));
		}
#endif
		// Keep going; file sink will likely fail and be reported below.
	}

	try {
		std::vector<spdlog::sink_ptr> sinks;

		if (!is_service_mode) {
			sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		}

		// 10MB x 5 files: logs/app.log, app.1.log, ...
		sinks.push_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>((logs_dir / "app.log").string(),
		                                                                       10 * 1024 * 1024, 5));

		auto logger = std::make_shared<spdlog::logger>("default", sinks.begin(), sinks.end());
		spdlog::set_default_logger(logger);
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
		spdlog::flush_on(spdlog::level::info);
	} catch (const std::exception& ex) {
#if defined(_WIN32)
		if (is_service_mode && !service_name.empty()) {
			winservice::report_event_error(service_name, L"Logging initialization failed: " +
			                                                 text::utf8_to_wide(std::string(ex.what())));
		}
#endif
		// As a last resort, keep default spdlog behavior (may be no-op in service).
	}
}

#if defined(_WIN32)
static int handle_windows_service_commands(const cxxopts::ParseResult& result) {
	const std::wstring svc_name = text::utf8_to_wide(std::string(APP_SERVICE_NAME));
	const std::wstring svc_display = text::utf8_to_wide(std::string(APP_SERVICE_DISPLAY_NAME));
	const std::wstring svc_desc = text::utf8_to_wide(std::string(APP_SERVICE_DESCRIPTION));

	if (result.count("service-install") > 0) {
		wchar_t exe_path[MAX_PATH]{};
		GetModuleFileNameW(nullptr, exe_path, MAX_PATH);
		std::wstring bin = L"\"" + std::wstring(exe_path) + L"\" --service";

		winservice::Options opt{svc_name, svc_display, svc_desc};
		std::wstring err;
		if (!winservice::install_service(opt, bin, &err)) {
			spdlog::error("Service install failed: {}", std::string(err.begin(), err.end()));
			return 1;
		}
		spdlog::info("Service installed: {}", APP_SERVICE_NAME);
		return 0;
	}

	if (result.count("service-uninstall") > 0) {
		std::wstring err;
		if (!winservice::uninstall_service(svc_name, &err)) {
			spdlog::error("Service uninstall failed: {}", std::string(err.begin(), err.end()));
			return 1;
		}
		spdlog::info("Service uninstalled: {}", APP_SERVICE_NAME);
		return 0;
	}

	return -1;
}
#endif

} // namespace

ServiceBootstrapResult bootstrap_service(const cxxopts::ParseResult& args) {
	ServiceBootstrapResult out;
#if defined(_WIN32)
	const bool is_service_mode = (args.count("service") > 0);
	if (is_service_mode) {
		out.service_name = text::utf8_to_wide(std::string(APP_SERVICE_NAME));
		winservice::report_event_info(out.service_name, L"Starting (service mode).");
	}

	out.command_exit_code = handle_windows_service_commands(args);
	if (out.command_exit_code != -1) {
		return out;
	}
#endif

	const std::filesystem::path cfg_path = exe_dir() / "config.json";
	out.config = load_config_from_file(cfg_path.string());

	const std::filesystem::path data_dir =
	    out.config.data_dir.empty() ? default_data_dir() : std::filesystem::path(out.config.data_dir);
	const std::filesystem::path logs_dir = data_dir / "logs";

	std::error_code ec;
	std::filesystem::create_directories(data_dir, ec);
	std::filesystem::create_directories(logs_dir, ec);

#if defined(_WIN32)
	init_logging(logs_dir, (args.count("service") > 0), out.service_name);
#else
	init_logging(logs_dir, false);
#endif

	spdlog::info("Using data_dir: {}", data_dir.string());

#if defined(_WIN32)
	if (!is_service_mode && args.count("service") > 0) {
		out.service_name = text::utf8_to_wide(std::string(APP_SERVICE_NAME));
	}
#endif

	return out;
}
