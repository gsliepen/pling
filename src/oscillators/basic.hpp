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
		return sinf(phase * float(2 * M_PI));
	}

	float fast_sine() {
		// Approximation of a sine using a parabola, without using branches.
		const float x1 = phase - 0.5f;
		const float x2 = fabsf(x1) * 4.0f - 1.0f;
		const float v = 1.0f - x2 * x2;
		return copysignf(v, x1);
	}

	float square() {
		return rintf(phase) * -2.0f + 1.0f;
	}

	float saw() {
		return phase * -2.0f + 1.0f;
	}

	float triangle() {
		return fabsf(phase - 0.5f) * 4.0f - 1.0f;
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

	float get_frequency(float bend = 1.0) {
		return delta * sample_rate * bend;
	}
};

}
