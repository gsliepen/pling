/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>

#include "program.hpp"

class ProgramManager {
	static constexpr uint32_t N = 16; // maximum number of simultaneously active programs

	// Added to by MIDI thread, deleted from by audio thread.
	std::atomic<Program *> active_programs[N]{};

	struct Entry {
		uint8_t MIDI_program;
		uint8_t bank_lsb;
		uint8_t bank_msb;
		std::shared_ptr<Program> program;

		Entry(uint8_t MIDI_program, uint8_t bank_lsb, uint8_t bank_msb, std::shared_ptr<Program> program):
			MIDI_program(MIDI_program),
			bank_lsb(bank_lsb),
			bank_msb(bank_msb),
			program(program)
		{}
	};

	std::deque<Entry> entries; // Only accessed by MIDI thread.

	public:
	/**
	 * Activate a Program for a given MIDI program.
	 *
	 * This returns a shared_ptr so we don't delete the program until everyone is done with it.
	 * We keep a reference ourselves as well. If we detect that no others are using the program,
	 * and the program itself is no longer actively producing sound, we delete it.
	 */
	std::shared_ptr<Program> activate(uint8_t MIDI_program, uint8_t bank_lsb = 0, uint8_t bank_msb = 0);

	void change(std::shared_ptr<Program> &program, uint8_t MIDI_program, uint8_t bank_lsb = 0, uint8_t bank_msb = 0);

	float get_zero_crossing(float offset);
	void render(Chunk &chunk);
};
