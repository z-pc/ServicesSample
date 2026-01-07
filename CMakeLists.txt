cmake_minimum_required(VERSION 3.20)

project(app_services LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# vcpkg manifest mode
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

add_executable(app_services
  src/main.cpp
  src/config.cpp
  src/service_bootstrap.cpp
  src/http_server.cpp
  src/api_router.cpp
  src/handlers.cpp
  src/handlers.h
  src/handlers/sample.h
  src/service_bootstrap.h
)

# Copy config.json next to the built executable.
# Note: this is implemented as a target POST_BUILD step to avoid generating an extra VS project.
add_custom_command(TARGET app_services POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${PROJECT_SOURCE_DIR}/config.json"
          "$<TARGET_FILE_DIR:app_services>/config.json"
  VERBATIM
)

if(WIN32)
  target_sources(app_services PRIVATE   
  ../src/windows_service.h
  ../src/windows_service.cpp)
endif()

target_include_directories(app_services PRIVATE src)
target_include_directories(app_services PRIVATE ${PROJECT_SOURCE_DIR}/external)

target_link_libraries(app_services PRIVATE
  nlohmann_json::nlohmann_json
  spdlog::spdlog
  fmt::fmt
  cxxopts::cxxopts
)

if(MSVC)
  target_compile_options(app_services PRIVATE /W4)
else()
  target_compile_options(app_services PRIVATE -Wall -Wextra -Wpedantic)
endif()
