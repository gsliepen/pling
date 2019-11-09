/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>

#include "../pling.hpp"

namespace Envelope {

class ExponentialADSR {
	float amplitude{};
	const float cutoff = 1.0e-4;
	enum class State {
		off,
		attack,
		decay,
		release,
	} state;

	public:
	struct Parameters {
		float attack{1};
		float decay;
		float sustain{1};
		float release;

		void set(float attack, float decay, float sustain, float release) {
			set_attack(attack);
			set_decay(decay);
			set_sustain(sustain);
			set_release(release);
		}

		void set_attack(float attack) {
			this->attack = sample_rate * attack > 1.0 ? 1.0 / (sample_rate * attack) : 1.0;
		}

		void set_decay(float decay) {
			this->decay = exp10(-2 / (decay * sample_rate));
		}

		void set_sustain(float sustain) {
			this->sustain = sustain;
		}

		void set_release(float release) {
			this->release = exp10(-2 / (sample_rate * release));
		}
	};

	void init() {
		amplitude = {}; state = State::attack;
	}

	bool is_active() {
		return state != State::off;
	}

	void release() {
		state = State::release;
	}

	float update(Parameters &param);
};

}
