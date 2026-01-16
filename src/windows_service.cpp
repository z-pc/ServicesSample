#if defined(_WIN32)

#include "windows_service.h"

#include <cstdint>
#include <exception>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "utf8.h"

namespace winservice {

static std::wstring win_err(DWORD code) {
	LPWSTR buffer = nullptr;
	const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
	const DWORD lang = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
	const DWORD size = FormatMessageW(flags, nullptr, code, lang, (LPWSTR)&buffer, 0, nullptr);
	std::wstring msg = size ? std::wstring(buffer, size) : L"Unknown error";
	if (buffer) LocalFree(buffer);
	return msg;
}

static void set_error(std::wstring* out, const std::wstring& message) {
	if (out) *out = message;
}

static SERVICE_STATUS_HANDLE g_status_handle = nullptr;
static SERVICE_STATUS g_status{};

static HANDLE g_stop_event = nullptr;

static int (*g_run_callback_v2)(void* context, void* stop_event) = nullptr;
static void* g_run_context = nullptr;

static std::wstring g_service_name;

static void report_event_impl(WORD type, const std::wstring& msg) {
	if (g_service_name.empty()) return;
	HANDLE src = RegisterEventSourceW(nullptr, g_service_name.c_str());
	if (!src) return;
	LPCWSTR strings[1] = {msg.c_str()};
	ReportEventW(src, type, 0, 0, nullptr, 1, 0, strings, nullptr);
	DeregisterEventSource(src);
}

void report_event_info(const std::wstring& service_name, const std::wstring& message) {
	g_service_name = service_name;
	report_event_impl(EVENTLOG_INFORMATION_TYPE, message);
}

void report_event_error(const std::wstring& service_name, const std::wstring& message) {
	g_service_name = service_name;
	report_event_impl(EVENTLOG_ERROR_TYPE, message);
}

static void report_status(DWORD state, DWORD win32_exit_code = NO_ERROR, DWORD wait_hint_ms = 0) {
	g_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_status.dwCurrentState = state;
	g_status.dwWin32ExitCode = win32_exit_code;
	g_status.dwWaitHint = wait_hint_ms;
	g_status.dwControlsAccepted = (state == SERVICE_RUNNING) ? (SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN) : 0;
	g_status.dwServiceSpecificExitCode = 0;
	g_status.dwCheckPoint = 0;
	SetServiceStatus(g_status_handle, &g_status);
}

static VOID WINAPI service_ctrl_handler(DWORD control) {
	switch (control) {
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		report_status(SERVICE_STOP_PENDING, NO_ERROR, 2000);
		if (g_stop_event) SetEvent(g_stop_event);
		break;
	default:
		break;
	}
}

static DWORD run_callback_guarded() {
	int run_rc = 0;

	try {
		report_event_impl(EVENTLOG_INFORMATION_TYPE, L"Service starting");
		report_status(SERVICE_RUNNING);

		if (g_run_callback_v2) {
			run_rc = g_run_callback_v2(g_run_context, g_stop_event);
		}

		if (run_rc != 0) {
			report_event_impl(EVENTLOG_ERROR_TYPE,
			                  L"Service runtime returned non-zero exit code: " + std::to_wstring(run_rc));
		}
	} catch (const std::exception& ex) {
		report_event_impl(EVENTLOG_ERROR_TYPE, L"Unhandled exception: " + text::utf8_to_wide(std::string(ex.what())));
		run_rc = 1;
	} catch (...) {
		report_event_impl(EVENTLOG_ERROR_TYPE, L"Unhandled non-standard exception");
		run_rc = 1;
	}

	return static_cast<DWORD>(run_rc);
}

static VOID WINAPI service_main(DWORD /*argc*/, LPWSTR* /*argv*/) {
	g_status_handle = RegisterServiceCtrlHandlerW(g_service_name.c_str(), service_ctrl_handler);
	if (!g_status_handle) return;

	g_stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!g_stop_event) {
		report_status(SERVICE_STOPPED, GetLastError());
		return;
	}

	report_status(SERVICE_START_PENDING, NO_ERROR, 2000);

	DWORD worker_rc = 0;
	std::thread worker([&]() { worker_rc = run_callback_guarded(); });

	const HANDLE wait_handles[2] = {g_stop_event, worker.native_handle()};
	(void)WaitForMultipleObjects(2, wait_handles, FALSE, INFINITE);

	if (worker.joinable()) worker.join();

	CloseHandle(g_stop_event);
	g_stop_event = nullptr;

	if (worker_rc != 0) {
		g_status.dwServiceSpecificExitCode = worker_rc;
		report_status(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
		return;
	}

	report_status(SERVICE_STOPPED);
}

int run_service(const std::wstring& service_name, int (*run_callback)(void* context, void* stop_event), void* context) {
	report_event_info(service_name, L"Service is starting");
	g_service_name = service_name;

	g_run_callback_v2 = run_callback;
	g_run_context = context;

	SERVICE_TABLE_ENTRYW table[2]{};
	table[0].lpServiceName = const_cast<LPWSTR>(g_service_name.c_str());
	table[0].lpServiceProc = service_main;
	table[1].lpServiceName = nullptr;
	table[1].lpServiceProc = nullptr;

	if (!StartServiceCtrlDispatcherW(table)) {
		report_event_impl(EVENTLOG_ERROR_TYPE, L"StartServiceCtrlDispatcherW failed: " + win_err(GetLastError()));
		return 1;
	}
	return 0;
}

} // namespace winservice

#endif
