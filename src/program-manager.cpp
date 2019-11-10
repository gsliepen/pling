/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "program-manager.hpp"
#include "programs/simple.hpp"

std::shared_ptr<Program> ProgramManager::activate(uint8_t MIDI_program, uint8_t bank_lsb, uint8_t bank_msb) {
	// Search known programs
	for (auto it = entries.begin(); it != entries.end();) {
		auto &entry = *it;

		if (entry.MIDI_program == MIDI_program && entry.bank_lsb == bank_lsb && entry.bank_msb == bank_msb) {
			return entry.program;
		}

		if (entry.program.use_count() == 1) {
			fprintf(stderr, "Removing old program %u %p\n", entry.MIDI_program, (void *)entry.program.get());
			it = entries.erase(it);
		} else
			++it;
	}

	auto &entry = entries.emplace_back(MIDI_program, bank_lsb, bank_msb, std::make_shared<Simple>());
	for (auto &program: active_programs) {
		if (!program) {
			program = entry.program.get();
			break;
		}
	}
	fprintf(stderr, "Activating new program %u %p\n", MIDI_program, (void *)entry.program.get());
	return entry.program;
}

void ProgramManager::change(std::shared_ptr<Program> &ptr, uint8_t MIDI_program, uint8_t bank_lsb, uint8_t bank_msb) {
	if(ptr.use_count() == 2) {
		Program *program = ptr.get();
		program->release_all();

		for (auto &active: active_programs) {
			if (active == program)
				active = {};
		}
	}

	ptr.reset();
	ptr = activate(MIDI_program, bank_lsb, bank_msb);
}

float ProgramManager::get_zero_crossing(float offset) {
	if (!entries.empty())
		return entries.front().program->get_zero_crossing(offset);
	else
		return offset;
}

void ProgramManager::render(Chunk &chunk) {
	chunk.clear();

	for (auto &program: active_programs) {
		Program *p = program;
		if (p)
			p->render(chunk);
	}
}
