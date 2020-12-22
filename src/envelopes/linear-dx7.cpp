/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "linear-adsr.hpp"

namespace Envelope
{

float LinearDX7::update(const Parameters &param)
{
	if (state < 3) {
		amplitude += param.rate[state];

		if ((param.rate[state] < 0 && amplitude < param.level[state]) || amplitude > param.level[state]))
			amplitude = param.level[state];

			state++;
		}
}

if ()
	switch (state)
	{
	case State::off:
		amplitude = 0;
		break;

	case State::attack1:
		amplitude += param.attack;

		if (amplitude >= 1) {
			amplitude = 1;
			state = State::decay;
		}

		break;

	case State::attack2:
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
