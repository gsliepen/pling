/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>

#include "midi.hpp"
#include "program.hpp"

class State
{
	using clock = std::chrono::steady_clock;

	MIDI::Port *active_port{};
	uint8_t active_channel{};
	uint8_t active_cc{};
	clock::time_point last_active_cc_change;
	std::shared_ptr<Program> active_program{};
	std::array<uint8_t, 128> keys;
	int16_t bend;
	float master_volume{1};
	bool learn_midi{};

	enum class Mode {
		INSTRUMENT,
		MIXER,
		PRESET,
	} mode{Mode::INSTRUMENT};

	void set_fader(MIDI::Control control, MIDI::Port &port, int8_t value);
	void set_pot(MIDI::Control control, MIDI::Port &port, int8_t value);
	void set_button(MIDI::Control control, MIDI::Port &port, int8_t value);
	void set_pitch_bend(MIDI::Control control, MIDI::Port &port, int8_t value);
	void set_modulation(MIDI::Control control, MIDI::Port &port, int8_t value);
	void set_sustain(MIDI::Control control, MIDI::Port &port, int8_t value);

public:
	void process_control(MIDI::Control control, MIDI::Port &port, const uint8_t *data, ssize_t len);

	void set_active_channel(MIDI::Port &port, uint8_t channel)
	{
		active_port = &port;
		active_channel = channel;
		set_active_cc(0);
	}

	void set_active_program(std::shared_ptr<Program> program)
	{
		active_program = program;
		set_active_cc(0);
	}

	void set_active_cc(uint8_t cc)
	{
		active_cc = cc;
		last_active_cc_change = clock::now();
	}

	void note_on(uint8_t key, uint8_t vel)
	{
		keys[key] = vel;
	}

	void note_off(uint8_t key)
	{
		keys[key] = 0;
	}

	void release_all()
	{
		keys.fill(0);
	}

	void set_bend(int16_t value)
	{
		bend = value;
	}

	void set_master_volume(float value)
	{
		master_volume = value;
	}

	void set_learn_midi(bool value)
	{
		learn_midi = value;
	}

	std::pair<MIDI::Port *, uint8_t> get_active_channel() const
	{
		return std::make_pair(active_port, active_channel);
	}

	std::shared_ptr<Program> get_active_program() const
	{
		return active_program;
	}

	uint8_t get_active_cc() const
	{
		return active_cc;
	}

	clock::time_point get_last_active_cc_change() const
	{
		return last_active_cc_change;
	}

	const std::array<uint8_t, 128> &get_keys() const
	{
		return keys;
	}

	int16_t get_bend() const
	{
		return bend;
	}

	float get_master_volume() const
	{
		return master_volume;
	}

	bool get_learn_midi() const
	{
		return learn_midi;
	}
};

extern State state;
