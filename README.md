# test-api-service

 Cross-platform C++17 API service.

## Dependencies (vcpkg manifest)
- `nlohmann-json`
- `cpp-httplib` (as `httplib` port)
- `spdlog`, `fmt`
- `cxxopts`
- `athread`

## Build (CMake + vcpkg)

```bash
# from repo root
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Run

```bash
./build/test_api_service --config config.json
```

## Endpoints
- `GET /health` -> `{ "status": "ok" }`
- `POST /echo` -> `{ "len": <n>, "body": "..." }`
