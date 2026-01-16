
# ServicesSample (app_services)

A small C++17 HTTP service sample.

## What’s inside

- HTTP server: `cpp-httplib` (header-only, see [external/httplib.h](external/httplib.h))
- JSON: `nlohmann::json`
- Logging: `spdlog` (console + rotating file)
- CLI parsing: `cxxopts`

The service now exposes basic observability endpoints by default:

- `GET /healthz`
- `GET /status`

## Requirements

- CMake >= 3.20
- A C++17 compiler (GCC / Clang / MSVC)

You also need these libraries discoverable by CMake via `find_package(... CONFIG REQUIRED)`:
	- `nlohmann-json`
	- `spdlog`
	- `fmt`
	- `cxxopts`

## Build (Linux/macOS)

### 1) Install dependencies

Install the dependencies using your system package manager (or build/install them yourself) so that CMake can find their *Config* packages.

Example (Ubuntu/Debian):

```bash
sudo apt update
sudo apt install -y \
  nlohmann-json3-dev \
  libspdlog-dev \
  libfmt-dev \
  libcxxopts-dev
```

If your distro does not provide `cxxopts` as a CMake package, build and install it from source, then set `CMAKE_PREFIX_PATH` to the install prefix when configuring.

### 2) Configure + build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Note: the CMake build adds a `POST_BUILD` step that copies [config.json](config.json) next to the built executable.

## Build (Windows) with vcpkg toolchain

This project uses `find_package(... CONFIG REQUIRED)`. On Windows, the simplest way to satisfy those dependencies is to use vcpkg in *toolchain* mode.

### 1) Install vcpkg

- Clone and bootstrap vcpkg (see vcpkg docs).
- Set an environment variable `VCPKG_ROOT` pointing to your vcpkg folder.

### 2) Install dependencies

From a Developer PowerShell:

```powershell
cd $env:VCPKG_ROOT
./vcpkg install nlohmann-json spdlog fmt cxxopts
```

If you want a specific architecture, add a triplet, for example:

```powershell
./vcpkg install nlohmann-json spdlog fmt cxxopts --triplet x64-windows
```

### 3) Configure + build with the vcpkg toolchain

From the repository root:

```powershell
cmake -S . -B build \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"

cmake --build build --config Release
```

Note: the build copies [config.json](config.json) next to the built executable.

## Run

Linux/macOS:

```bash
cd build
./app_services
```

Windows (example for CMake multi-config generators):

```powershell
.\build\Release\app_services.exe
```

You should see logs like:

- `Using data_dir: ...`
- `Listening on 0.0.0.0:8080 (threads=...)`
- Per-request logs (non-observability paths): `GET /unknown 404 remote_addr=...`

## Configuration

The service loads `config.json` from:

- Windows: the executable directory
- Linux/macOS: the current working directory

If you start the binary from a different directory (not the one containing `config.json`), config loading will fail.

Supported fields:

- `host` (string): bind address, default `0.0.0.0`
- `port` (int): listen port, default `8080`
- `threads` (number): request worker threads
	- `0` means “auto” (uses `std::thread::hardware_concurrency()`)
- `data_dir` (string, optional): data directory
	- if empty, defaults to:
		- Linux/macOS: `./data`
		- Windows: `%ProgramData%/ServicesSample`

Example [config.json](config.json):

```json
{
	"host": "0.0.0.0",
	"port": 8080,
	"threads": 6
}
```

## Logs

On startup, the app creates:

- `data_dir/`
- `data_dir/logs/`

And writes a rotating log file:

- `data_dir/logs/app.log` (10MB x 5 files)

If you don’t set `data_dir` on Linux/macOS, logs will be under:

- `./data/logs/app.log`

## API

## Adding endpoints (handlers) & middleware

Routing is wrapped by `ApiRouter` (see [src/api_router.h](src/api_router.h)). Register endpoints inside `register_handlers(ApiRouter& api)` (see [src/handlers.cpp](src/handlers.cpp)).

### Implementing a handler

A handler has the signature:

```cpp
using Handler = std::function<void(const httplib::Request&, httplib::Response&)>;
```

Example endpoint `GET /hello` (already present):

```cpp
api.get("/hello", [](const httplib::Request& req, httplib::Response& res) {
	nlohmann::json j;
	j["message"] = "Hello, World!";
	res.status = 200;
	res.set_content(j.dump(), "application/json");
});
```

Available route registration helpers:

- `api.get(path, handler)`
- `api.post(path, handler)`
- `api.put(path, handler)`
- `api.del(path, handler)`

Quick notes for handlers:

- Set `res.status` before returning.
- Use `res.set_content(body, content_type)` to set the response body.
- If handlers can throw, the router supports centralized error handling via `set_exception_handler(...)` (see `ApiRouter`).

### Grouping routes by prefix

To group endpoints under a prefix (e.g. `/api/v1`), use `group(prefix, define_routes)`:

```cpp
api.group("/api/v1", [](ApiRouter& r) {
	r.get("/ping", [](const httplib::Request&, httplib::Response& res) {
		res.status = 200;
		res.set_content("pong", "text/plain; charset=utf-8");
	});
});
// -> GET /api/v1/ping
```

### Implementing middleware

Middleware has the signature:

```cpp
using Next = std::function<void()>;
using Middleware = std::function<void(const httplib::Request&, httplib::Response&, Next)>;
```

There are two kinds:

- `api.use(mw)` runs *before* the route handler
- `api.use_after(mw)` runs *after* the route handler

Middleware runs as a chain. If you want the request to continue to the route handler (or the next middleware), you **must call `next()`**. If you set a response and *don’t* call `next()`, the request stops there (commonly used for auth/deny).

#### Example: timing + access log (before)

```cpp
api.use([](const httplib::Request& req, httplib::Response& res, ApiRouter::Next next) {
	const auto start = std::chrono::steady_clock::now();
	next();
	const auto end = std::chrono::steady_clock::now();
	const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	spdlog::info("{} {} took {}ms status={}", req.method, req.path, ms, res.status);
});
```

#### Example: simple auth (before, blocks request)

```cpp
api.use([](const httplib::Request& req, httplib::Response& res, ApiRouter::Next next) {
	auto it = req.headers.find("x-api-key");
	if (it == req.headers.end() || it->second != "secret") {
		res.status = 401;
		res.set_content("unauthorized", "text/plain; charset=utf-8");
		return; // no next() => stop
	}
	next();
});
```

#### Example: add a response header (after)

```cpp
api.use_after([](const httplib::Request&, httplib::Response& res, ApiRouter::Next next) {
	next();
	res.set_header("x-service", "app_services");
});
```

For a real example used in this repo, see the observability middleware in [src/observability.cpp](src/observability.cpp).

### GET /healthz

Returns plain text:

```text
ok
```

Quick test:

```bash
curl -s http://127.0.0.1:8080/healthz
```

### GET /status

Returns a small JSON payload (counters are process-wide):

```json
{"requests_total":123,"requests_in_flight":0}
```

Quick test:

```bash
curl -s http://127.0.0.1:8080/status
```

### GET /metrics (disabled by default)

Prometheus text format endpoint. Currently disabled in handler registration.

### GET /trace (disabled by default)

Returns tracing/correlation headers (and echoes them in the body). Currently disabled in handler registration.

### 404

Unknown routes return:

```json
{"error":"not found"}
```

## Windows Service (Windows only)

Supported options:

- `--service` run as Windows Service (used by the installer-created service)

Note: Service recovery (automatic restarts on failure) is configured by the installer script.
Control Manager defaults baked into the app.

## Troubleshooting

- `Unable to open config file`: ensure `config.json` is in the directory the app resolves it from (see “Configuration”).
- Port already in use: change `port` in `config.json`.

