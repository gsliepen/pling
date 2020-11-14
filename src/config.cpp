/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "config.hpp"

#include <filesystem>

namespace fs = std::filesystem;

void Config::init(const fs::path &pref_path) {
	config = YAML::LoadFile(pref_path / "config.yaml");
	data_dir = config["data_dir"].as<std::string>(data_dir);
	local_dir = config["local_dir"].as<std::string>(pref_path / "data");
}

fs::path Config::get_load_path(const fs::path &filename) {
	auto local_path = data_dir / filename;

	if (fs::exists(local_path))
		return local_path;

	return data_dir / filename;
}

