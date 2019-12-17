/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <glm/glm.hpp>
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

		void set(Type type, float freq, float Q) {
			this->type = type;
			this->f = glm::clamp(2.0f * sinf(float(M_PI) * freq / sample_rate), 0.0f, 1.0f);
			this->q = glm::clamp(1.0f / Q, 0.0f, 1.0f);
		}

		void set_freq(float freq) {
			this->f = glm::clamp(2.0f * sinf(float(M_PI) * freq / sample_rate), 0.0f, 1.0f);
		}

		bool build_widget(const std::string &name);
	};

	float filter(Parameters &params, float in) {
		low  = params.f * band + low;
		high = in - params.q * band - low;
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
