/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "biquad.hpp"
#include "../pling.hpp"

#include <cmath>

namespace Filter {

void Biquad::Parameters::set(Type type, float freq, float Q, float gain) {
	float norm;
	float V = pow(10, fabs(gain) / 20.0);
	float K = tan(M_PI * freq / sample_rate);

	switch (type) {
	case Type::lowpass:
		norm = 1 / (1 + K / Q + K * K);
		a0 = K * K * norm;
		a1 = 2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case Type::highpass:
		norm = 1 / (1 + K / Q + K * K);
		a0 = 1 * norm;
		a1 = -2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case Type::bandpass:
		norm = 1 / (1 + K / Q + K * K);
		a0 = K / Q * norm;
		a1 = 0;
		a2 = -a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case Type::peak:
		if (gain >= 0) {
			norm = 1 / (1 + 1 / Q * K + K * K);
			a0 = (1 + V / Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - V / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - 1 / Q * K + K * K) * norm;
		} else {
			norm = 1 / (1 + V / Q * K + K * K);
			a0 = (1 + 1 / Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - 1 / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - V / Q * K + K * K) * norm;
		}
		break;

	case Type::notch:
		norm = 1 / (1 + K / Q + K * K);
		a0 = (1 + K * K) * norm;
		a1 = 2 * (K * K - 1) * norm;
		a2 = a0;
		b1 = a1;
		b2 = (1 - K / Q + K * K) * norm;
		break;

	case Type::highshelf:
		if (gain >= 0) {
			norm = 1 / (1 + sqrtf(2) * K + K * K);
			a0 = (V + sqrtf(2 * V) * K + K * K) * norm;
			a1 = 2 * (K * K - V) * norm;
			a2 = (V - sqrtf(2 * V) * K + K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrtf(2) * K + K * K) * norm;
		} else {
			norm = 1 / (V + sqrtf(2 * V) * K + K * K);
			a0 = (1 + sqrtf(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrtf(2) * K + K * K) * norm;
			b1 = 2 * (K * K - V) * norm;
			b2 = (V - sqrtf(2 * V) * K + K * K) * norm;
		}
		break;

	case Type::lowshelf:
		if (gain >= 0) {
			norm = 1 / (1 + sqrtf(2) * K + K * K);
			a0 = (1 + sqrtf(2 * V) * K + V * K * K) * norm;
			a1 = 2 * (V * K * K - 1) * norm;
			a2 = (1 - sqrtf(2 * V) * K + V * K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrtf(2) * K + K * K) * norm;
		} else {
			norm = 1 / (1 + sqrtf(2 * V) * K + V * K * K);
			a0 = (1 + sqrtf(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrtf(2) * K + K * K) * norm;
			b1 = 2 * (V * K * K - 1) * norm;
			b2 = (1 - sqrtf(2 * V) * K + V * K * K) * norm;
		}
		break;
	}

	return;
}

}
