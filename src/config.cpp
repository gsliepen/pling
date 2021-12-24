/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "config.hpp"

#include <filesystem>
#include <fmt/ostream.h>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

void Config::init(const fs::path &pref_path)
{
	if (!fs::exists(pref_path) && !fs::create_directories(pref_path)) {
		throw std::runtime_error("Could not access or create config directory " + pref_path.native());
	}

	auto filename = pref_path / "config.yaml";
	std::ifstream file(filename, std::ios_base::app);

	if (!file) {
		throw std::runtime_error("Could not open or create config file " + filename.native());
	}

	config = YAML::Load(file);
	auto default_data_dir = data_dir;
	data_dir = config["data_dir"].as<std::string>(data_dir);
	local_dir = config["local_dir"].as<std::string>(pref_path / "data");
	cache_dir = config["cache_dir"].as<std::string>(pref_path / "cache");

	/* The default data directory might not exist.
	   Try to fallback to a local data directory if possible,
	   to get a better out-of-the-box experience. */
	if (!fs::exists(data_dir)) {
		if (data_dir == default_data_dir) {
			for (auto path = fs::current_path(); path != path.root_path(); path = path.parent_path()) {
				if (fs::exists(path / "data" / "controllers" / "default")) {
					data_dir = path / "data";
					break;
				}
			}
		}

		if (data_dir == DATADIR) {
			fmt::print(std::cerr, "Data directory {} not valid!\n", default_data_dir);
		} else {
			fmt::print(std::cerr, "Data directory {} not valid, using fallback {}\n", default_data_dir, data_dir);
		}
	}
}

fs::path Config::get_load_path(const fs::path &filename)
{
	if (auto local_path = local_dir / filename; fs::exists(local_path)) {
		return local_path;
	} else {
		return data_dir / filename;
	}
}

fs::path Config::get_save_path(const fs::path &filename)
{
	auto path = local_dir / filename;

	if (!fs::exists(path)) {
		fs::create_directories(path.parent_path());
	}

	return path;
}

fs::path Config::get_cache_path(const fs::path &filename)
{
	auto path = cache_dir / filename;

	if (!fs::exists(path)) {
		fs::create_directories(path.parent_path());
	}

	return path;
}
