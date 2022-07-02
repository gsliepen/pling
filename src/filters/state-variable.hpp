/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <string>
#include <glm/glm.hpp>
#include "../pling.hpp"

namespace Filter
{

class StateVariable
{
	float low{};
	float band{};
	float high{};

	float low24{};
	float band24{};
	float high24{};

public:
	struct Parameters {
		enum class Type {
			none,
			lowpass,
			lowpass24,
			highpass,
			highpass24,
			bandpass,
			bandpass24,
			notch,
			notch24,
		} type{};

		float f{1};
		float q{1};

		void set(Type type, float freq, float Q)
		{
			this->type = type;
			this->f = 2.0f * std::sin(glm::clamp(float(M_PI) * freq / sample_rate, 0.0f, std::asin(0.5f)));
			this->q = glm::clamp(1.0f / Q, 0.0f, 1.0f);
		}

		void set_freq(float freq)
		{
			this->f = 2.0f * std::sin(glm::clamp(float(M_PI) * freq / sample_rate, 0.0f, std::asin(0.5f)));
		}

		bool build_widget(const std::string &name);
	};

	float filter(Parameters &params, float in)
	{
		if (params.type == Parameters::Type::none) {
			return in;
		}

		low  = params.f * band + low;
		high = in - params.q * band - low;
		band = params.f * high + band;

		switch (params.type) {
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

		case Parameters::Type::lowpass24:
			low24  = params.f * band24 + low24;
			high24 = low - band24 - low24;
			band24 = params.f * high24 + band24;
			return low24;
			break;

		case Parameters::Type::highpass24:
			low24  = params.f * band24 + low24;
			high24 = high - band24 - low24;
			band24 = params.f * high24 + band24;
			return high24;
			break;

		case Parameters::Type::bandpass24:
			low24  = params.f * band24 + low24;
			high24 = band - band24 - low24;
			band24 = params.f * high24 + band24;
			return band24;
			break;

		case Parameters::Type::notch24:
			low24  = params.f * band24 + low24;
			high24 = high + low - band24 - low24;
			band24 = params.f * high24 + band24;
			return high24 + low24;
			break;

		default:
			return {};
		}
	}

	float operator()(Parameters &params, float in)
	{
		return filter(params, in);
	}
};

}
