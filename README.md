# app_

 Cross-platform C++17 API service.

## Dependencies (vcpkg manifest)
- `nlohmann-json`
- `cpp-httplib` (as `httplib` port)
- `spdlog`, `fmt`
- `cxxopts`

## Build (CMake + vcpkg)

```bash
# from repo root
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

## Run

```bash
./build/app_service
```

## Endpoints
- `GET /sample` -> `{ "status": "ok" }`
