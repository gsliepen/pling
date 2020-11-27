/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

namespace Filter
{

class Biquad
{
	float z1{};
	float z2{};

public:
	struct Parameters {
		enum class Type {
			lowpass,
			highpass,
			bandpass,
			peak,
			notch,
			highshelf,
			lowshelf,
		};

		float a0{1};
		float a1{};
		float a2{};
		float b1{};
		float b2{};

		void set(Type type, float freq, float Q, float gain);
	};

	float filter(Parameters &params, float in)
	{
		float out = in * params.a0 + z1;
		z1 = in * params.a1 - params.b1 * out + z2;
		z2 = in * params.a2 - params.b2 * out;
		return out;
	}

	float operator()(Parameters &params, float in)
	{
		return filter(params, in);
	}
};

}
