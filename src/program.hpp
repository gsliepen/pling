/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "pling.hpp"

class Program {
	public:
	virtual ~Program() {};

	virtual void render(Chunk &chunk) {};
	virtual float get_zero_crossing(float offset) {
		return offset;
	};

	virtual void note_on(uint8_t key, uint8_t vel) {};
	virtual void note_off(uint8_t key, uint8_t vel) {};
	virtual void pitch_bend(int16_t bend) {};
	virtual void channel_pressure(int8_t pressure) {};
	virtual void poly_pressure(uint8_t key, uint8_t pressure) {};
	virtual void control_change(uint8_t control, uint8_t val) {};
};
