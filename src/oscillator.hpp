/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <glm/glm.hpp>

#include "pling.hpp"

class Oscillator {
	private:
	float delta{};
	float phase{};

	public:
	Oscillator() = default;

	Oscillator(float freq) {
		init(freq);
	}

	void init(float freq) {
		this->delta = freq / sample_rate;
		this->phase = {};
	}

	void update() {
		phase += delta;
		if (phase >= 1)
			phase -= 1;
	};

	void update(float bend) {
		phase += delta * bend;
		if (phase >= 1) {
			phase -= 1;
		}
	};

	float get() {
		return sinf(phase * 2 * M_PI);
	}

	float square() {
		return phase < 0.5 ? 1 : -1;
	}

	Oscillator &operator++() {
		update();
		return *this;
	}

	operator float() {
		return get();
	}

	float get_zero_crossing(float offset, float bend = 1.0) {
		/* Going backwards from the current sample position + offset,
		 * return the first zero crossing of the phase of this oscillator. */

		float phase_at_offset = phase + offset * delta * bend;
		return offset - glm::fract(phase_at_offset) / (delta * bend);
	}
};

