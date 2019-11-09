/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>

#include "../pling.hpp"

namespace Oscillator {

class Basic {
	private:
	float delta{};
	float phase{};

	public:
	Basic() = default;

	Basic(float freq) {
		init(freq);
	}

	void init(float freq) {
		this->delta = freq / sample_rate;
		this->phase = {};
	}

	void update() {
		phase += delta;
		phase -= floorf(phase);
	};

	void update(float bend) {
		phase += delta * bend;
		phase -= floorf(phase);
	};

	float sine() {
		return sinf(phase * 2 * M_PI);
	}

	float square() {
		return phase < 0.5 ? 1 : -1;
	}

	float saw() {
		return phase < 0.5 ? phase * 2 : phase * 2 - 2;
	}

	float triangle() {
		return phase < 0.25 ? phase * 4 : phase < 0.75 ? 2 - phase * 4 : phase * 4 - 4;
	}

	Basic &operator++() {
		update();
		return *this;
	}

	operator float() {
		return phase;
	}

	float get_zero_crossing(float offset, float bend = 1.0) {
		/* Going backwards from the current sample position + offset,
		 * return the first zero crossing of the phase of this oscillator. */

		float phase_at_offset = phase + offset * delta * bend;
		phase_at_offset -= floorf(phase_at_offset);
		return offset - phase_at_offset / (delta * bend);
	}
};

}
