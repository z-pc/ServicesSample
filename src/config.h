#pragma once

#include <string>

struct AppConfig {
	std::string host = "0.0.0.0";
	int port = 8080;
	std::size_t threads = 0; // 0 => auto
	std::string data_dir; // empty => use platform default
};

AppConfig load_config_from_file(const std::string& path);
