/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <string>

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

		float get_attack() {
			return attack >= 1.0f ? 0.0f : 1.0f / (sample_rate * attack);
		}

		float get_decay() {
			return -2.0f / log10(decay) / sample_rate;
		}

		float get_sustain() {
			return sustain;
		}

		float get_release() {
			return -2.0f / log10(release) / sample_rate;
		}

		bool build_widget(const std::string &name);
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
