#pragma once

#include <string>

/**
 * @brief Application runtime configuration.
 */
struct AppConfig {
	/// @brief TCP port to listen on.
	int port = 8080;

	/// @brief Worker thread count (0 => auto).
	std::size_t threads = 0;

	/// @brief Directory for application data (empty => platform default).
	std::string data_dir;
};

/**
 * @brief Load configuration from a file path.
 * @throws (implementation-defined) on parse/read failure.
 */
AppConfig load_config_from_file(const std::string& path);
