
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

- `--service` run as a Windows service
- `--service-install` install the service
- `--service-uninstall` uninstall the service

Example:

```powershell
app_services.exe --service-install
app_services.exe --service
```

Note: `--service-install` configures basic service recovery (automatic restarts on failure) using the Windows Service
Control Manager defaults baked into the app.

## Troubleshooting

- `Unable to open config file`: ensure `config.json` is in the directory the app resolves it from (see “Configuration”).
- Port already in use: change `port` in `config.json`.

