/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <algorithm>
#include "midi.hpp"
#include "program.hpp"

class View {
	MIDI::Port *active_port{};
	uint8_t active_channel{};
	std::shared_ptr<Program> active_program{};
	std::array<uint8_t, 128> keys;
	int16_t bend;

	public:
	void set_active_channel(MIDI::Port &port, uint8_t channel) {
		active_port = &port;
		active_channel = channel;
	}

	void set_active_program(std::shared_ptr<Program> program) {
		active_program = program;
	}

	void note_on(uint8_t key, uint8_t vel) {
		keys[key] = vel;
	}

	void note_off(uint8_t key) {
		keys[key] = 0;
	}

	void set_bend(int16_t value) {
		bend = value;
	}

	std::pair<MIDI::Port *, uint8_t> get_active_channel() {
		return std::make_pair(active_port, active_channel);
	}

	std::shared_ptr<Program> get_active_program() {
		return active_program;
	}

	const std::array<uint8_t, 128> &get_keys() {
		return keys;
	}

	int16_t get_bend() {
		return bend;
	}
};

extern View view;
