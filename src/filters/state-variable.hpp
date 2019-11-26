/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include "../pling.hpp"

namespace Filter {

class StateVariable {
	float low{};
	float band{};
	float high{};

	public:
	struct Parameters {
		enum class Type {
			lowpass,
			highpass,
			bandpass,
			notch,
		} type{};

		float f{1};
		float q{1};
		float sqrt_q{1};

		void set(Type type, float freq, float Q) {
			this->type = type;
			this->f = freq / sample_rate * 4;
			this->q = 1 - 2 / M_PI * atanf(sqrtf(Q));
			this->sqrt_q = sqrtf(q);
		}

		void set_freq(float freq) {
			this->f = freq / sample_rate * 4;
		}
	};

	float filter(Parameters &params, float in) {
		low  = params.f * band + low;
		high = params.sqrt_q * in - params.q * band - low;
		band = params.f * high + band;

		switch(params.type) {
			case Parameters::Type::lowpass:
				return low;
				break;
			case Parameters::Type::highpass:
				return high;
				break;
			case Parameters::Type::bandpass:
				return band;
				break;
			case Parameters::Type::notch:
				return high + low;
				break;
			default:
				return {};
		}
	}

	float operator()(Parameters &params, float in) {
		return filter(params, in);
	}
};

}
