/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "config.hpp"

#include <filesystem>

namespace fs = std::filesystem;

void Config::init(const fs::path &pref_path) {
	config = YAML::LoadFile(pref_path / "config.yaml");
	data_dir = config["data_dir"].as<std::string>(data_dir);
	local_dir = config["local_dir"].as<std::string>(pref_path / "data");
	cache_dir = config["cache_dir"].as<std::string>(pref_path / "cache");
}

fs::path Config::get_load_path(const fs::path &filename) {
	if (auto local_path = local_dir / filename; fs::exists(local_path))
		return local_path;
	else
		return data_dir / filename;
}

fs::path Config::get_save_path(const fs::path &filename) {
	auto path = local_dir / filename;
	if (!fs::exists(path)) {
		fs::create_directories(path.parent_path());
	}
	return path;
}

fs::path Config::get_cache_path(const fs::path &filename) {
	auto path = cache_dir / filename;
	if (!fs::exists(path)) {
		fs::create_directories(path.parent_path());
	}
	return path;
}
