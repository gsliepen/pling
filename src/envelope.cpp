/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "envelope.hpp"
#include "pling.hpp"

namespace Envelope {

float ADSR::update(Parameters &param, bool on) {
	const float dt = 1.0 / sample_rate;

	if (t < 0)
		return 0;

	t += dt;

	if (on) {
		if (t < param.attack) {
			amplitude = t / param.attack;
		} else if (t < param.attack + param.decay) {
			amplitude = 1 - (1 - param.sustain) * (t - param.attack) / param.decay;
		} else {
			amplitude = param.sustain;
		}
	} else {
		amplitude -= dt / (param.release * param.sustain);
		if (amplitude < 0) {
			amplitude = 0;
			t = -1;
		}
	}

	return amplitude;
}

}
