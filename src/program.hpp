/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "pling.hpp"

#include <yaml-cpp/yaml.h>

class Program {
	protected:
	bool active{};
	uint8_t MIDI_program;
	uint8_t bank_lsb;
	uint8_t bank_msb;

	std::string name;

	public:
	class Manager;

	virtual ~Program() {};

	virtual bool render(Chunk &chunk) {
		return false;
	};

	virtual float get_zero_crossing(float offset) {
		return offset;
	};

	virtual float get_base_frequency(void) {
		return {};
	};

	virtual void note_on(uint8_t key, uint8_t vel) {};
	virtual void note_off(uint8_t key, uint8_t vel) {};
	virtual void pitch_bend(int16_t bend) {};
	virtual void channel_pressure(int8_t pressure) {};
	virtual void poly_pressure(uint8_t key, uint8_t pressure) {};
	virtual void control_change(uint8_t control, uint8_t val) {};
	virtual void release_all() {};

	virtual bool build_cc_widget(uint8_t control) {
		(void)control;
		return false;
	};

	virtual bool load(const YAML::Node &yaml) {
		return false;
	};

	virtual YAML::Node save() {
		return {};
	};

	virtual const std::string &get_engine_name() {
		static const std::string name{"None"};
		return name;
	}

	const std::string &get_name() {
		return name;
	}

	const uint8_t get_MIDI_program() {
		return MIDI_program;
	}
};
