#include "config.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

AppConfig load_config_from_file(const std::string& path) {
	std::ifstream in(path);
	if (!in.is_open()) {
		throw std::runtime_error("Unable to open config file: " + path);
	}

	nlohmann::json j;
	in >> j;

	AppConfig cfg;
	if (j.contains("port")) cfg.port = j.at("port").get<int>();
	if (j.contains("threads")) cfg.threads = j.at("threads").get<std::size_t>();
	if (j.contains("data_dir")) cfg.data_dir = j.at("data_dir").get<std::string>();
	return cfg;
}
