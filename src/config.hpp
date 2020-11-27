/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <filesystem>
#include <yaml-cpp/yaml.h>

#include "config.h"

class Config
{
public:
	void init(const std::filesystem::path &pref_path);
	std::filesystem::path get_load_path(const std::filesystem::path &filename);
	std::filesystem::path get_save_path(const std::filesystem::path &filename);
	std::filesystem::path get_cache_path(const std::filesystem::path &filename);

	template<typename Key>
	auto operator[](const Key &key)
	{
		return config[key];
	}

private:
	std::filesystem::path data_dir = DATADIR;
	std::filesystem::path local_dir = "data";
	std::filesystem::path cache_dir = "data";
	YAML::Node config;
};
