/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "program-manager.hpp"

#include <filesystem>
#include <fmt/ostream.h>
#include <iostream>
#include <yaml-cpp/yaml.h>

#include "config.hpp"
#include "programs/simple.hpp"

void Program::Manager::activate(std::shared_ptr<Program> &program)
{
	last_activated_program = program;

	std::lock_guard lock(active_program_mutex);

	if (program->active) {
		return;
	}

	program->active = true;
	active_programs.push_back(program);
}

void Program::Manager::change(std::shared_ptr<Program> &program, uint8_t MIDI_program, uint8_t bank_lsb, uint8_t bank_msb)
{
	if (program) {
		if (program->MIDI_program == MIDI_program && program->bank_lsb == bank_lsb && program->bank_msb == bank_msb) {
			return;
		}

		program->release_all();
	}

	std::filesystem::path filename = "programs";
	filename /= "bank-" + std::to_string(bank_lsb << 7 | bank_msb);
	filename /= std::to_string(MIDI_program) + ".yaml";
	auto path = config.get_load_path(filename);

	try {
		auto program_config = YAML::LoadFile(path);

		auto engine_name = program_config["engine"].as<std::string>();

		if (const auto &it = engines.find(engine_name); it != engines.end()) {
			program = it->second();
			program->name = program_config["name"].as<std::string>();
			program->load(program_config["parameters"]);
		} else {
			program = std::make_shared<Program>();
			program->name = "Invalid program";
		}
	} catch (YAML::Exception &e) {
		fmt::print(std::cerr, "{} could not be parsed: {}\n", path, e.what());
		program = std::make_shared<Program>();
		program->name = "None";
	}

	program->MIDI_program = MIDI_program;
	program->bank_lsb = bank_lsb;
	program->bank_msb = bank_msb;

	selected_program = program;
	last_activated_program = program;
}

float Program::Manager::get_zero_crossing(float offset)
{
	if (last_activated_program) {
		return last_activated_program->get_zero_crossing(offset);
	} else {
		return offset;
	}
}

float Program::Manager::get_base_frequency()
{
	if (last_activated_program) {
		return last_activated_program->get_base_frequency();
	} else
		return {};
}

void Program::Manager::render(Chunk &chunk)
{
	chunk.clear();

	std::lock_guard lock(active_program_mutex);

	for (auto it = active_programs.begin(); it != active_programs.end();) {
		auto program = it->get();

		if (!program->render(chunk)) {
			program->active = false;
			it = active_programs.erase(it);
		} else {
			++it;
		}
	}
}
