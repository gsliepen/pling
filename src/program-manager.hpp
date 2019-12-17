/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "program.hpp"

class Program::Manager {
	// Added to by MIDI thread, deleted from by audio thread.
	using EngineFactory = std::function<std::shared_ptr<Program>()>;
	std::deque<std::shared_ptr<Program>> active_programs;
	std::mutex active_program_mutex;

	std::shared_ptr<Program> selected_program;
	std::shared_ptr<Program> last_activated_program;

	std::unordered_map<std::string, EngineFactory> engines;

	public:
	class Registration {};

	/**
	 * Activate a Program for a given MIDI program.
	 *
	 * This adds it to the list of active programs, which will cause their sound to be rendered.
	 */
	void activate(std::shared_ptr<Program> &program);

	void change(std::shared_ptr<Program> &program, uint8_t MIDI_program, uint8_t bank_lsb = 0, uint8_t bank_msb = 0);

	float get_zero_crossing(float offset);
	float get_base_frequency();
	void render(Chunk &chunk);

	std::shared_ptr<Program> get_selected_program() {
		return selected_program;
	}

	std::shared_ptr<Program> get_last_activated_program() {
		return last_activated_program;
	}

	Registration register_engine(const std::string &name, EngineFactory factory) {
		engines[name] = factory;
		return {};
	}
};

extern Program::Manager programs;
