/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "linear-adsr.hpp"

namespace Envelope
{

float LinearADSR::update(Parameters &param)
{
	switch (state) {
	case State::off:
		amplitude = 0;
		break;

	case State::attack:
		amplitude += param.attack;

		if (amplitude >= 1) {
			amplitude = 1;
			state = State::decay;
		}

		break;

	case State::decay:
		amplitude -= param.decay;

		if (amplitude < param.sustain) {
			amplitude = param.sustain;
		}

		break;

	case State::release:
		amplitude -= param.release;

		if (amplitude < cutoff) {
			amplitude = 0;
			state = State::off;
		}

		break;
	}

	return amplitude;
}

}
