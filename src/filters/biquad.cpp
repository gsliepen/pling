/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "biquad.hpp"
#include "../pling.hpp"

#include <cmath>

namespace Filter {

void Biquad::Parameters::set(Type type, float freq, float Q, float gain) {
	float norm;
	float V = std::pow(10.0f, std::abs(gain) / 20.0);
	float K = std::tan(M_PI * freq / sample_rate);

	switch (type) {
	case Type::lowpass:
		norm = 1.0f / (1.0f + K / Q + K * K);
		a0 = K * K * norm;
		a1 = 2.0f * a0;
		a2 = a0;
		b1 = 2.0f * (K * K - 1.0f) * norm;
		b2 = (1.0f - K / Q + K * K) * norm;
		break;

	case Type::highpass:
		norm = 1.0f / (1.0f + K / Q + K * K);
		a0 = 1.0f * norm;
		a1 = -2.0f * a0;
		a2 = a0;
		b1 = 2.0f * (K * K - 1.0f) * norm;
		b2 = (1.0f - K / Q + K * K) * norm;
		break;

	case Type::bandpass:
		norm = 1.0f / (1.0f + K / Q + K * K);
		a0 = K / Q * norm;
		a1 = 0;
		a2 = -a0;
		b1 = 2.0f * (K * K - 1.0f) * norm;
		b2 = (1.0f - K / Q + K * K) * norm;
		break;

	case Type::peak:
		if (gain >= 0) {
			norm = 1.0f / (1.0f + 1.0f / Q * K + K * K);
			a0 = (1.0f + V / Q * K + K * K) * norm;
			a1 = 2.0f * (K * K - 1.0f) * norm;
			a2 = (1.0f - V / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1.0f - 1.0f / Q * K + K * K) * norm;
		} else {
			norm = 1.0f / (1.0f + V / Q * K + K * K);
			a0 = (1.0f + 1.0f / Q * K + K * K) * norm;
			a1 = 2.0f * (K * K - 1.0f) * norm;
			a2 = (1.0f - 1.0f / Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1.0f - V / Q * K + K * K) * norm;
		}
		break;

	case Type::notch:
		norm = 1.0f / (1.0f + K / Q + K * K);
		a0 = (1.0f + K * K) * norm;
		a1 = 2.0f * (K * K - 1.0f) * norm;
		a2 = a0;
		b1 = a1;
		b2 = (1.0f - K / Q + K * K) * norm;
		break;

	case Type::highshelf:
		if (gain >= 0) {
			norm = 1.0f / (1.0f + std::sqrt(2.0f) * K + K * K);
			a0 = (V + std::sqrt(2.0f * V) * K + K * K) * norm;
			a1 = 2.0f * (K * K - V) * norm;
			a2 = (V - std::sqrt(2.0f * V) * K + K * K) * norm;
			b1 = 2.0f * (K * K - 1.0f) * norm;
			b2 = (1.0f - std::sqrt(2.0f) * K + K * K) * norm;
		} else {
			norm = 1.0f / (V + std::sqrt(2.0f * V) * K + K * K);
			a0 = (1.0f + std::sqrt(2.0f) * K + K * K) * norm;
			a1 = 2.0f * (K * K - 1.0f) * norm;
			a2 = (1.0f - std::sqrt(2.0f) * K + K * K) * norm;
			b1 = 2.0f * (K * K - V) * norm;
			b2 = (V - std::sqrt(2.0f * V) * K + K * K) * norm;
		}
		break;

	case Type::lowshelf:
		if (gain >= 0) {
			norm = 1.0f / (1.0f + std::sqrt(2.0f) * K + K * K);
			a0 = (1.0f + std::sqrt(2.0f * V) * K + V * K * K) * norm;
			a1 = 2.0f * (V * K * K - 1.0f) * norm;
			a2 = (1.0f - std::sqrt(2.0f * V) * K + V * K * K) * norm;
			b1 = 2.0f * (K * K - 1.0f) * norm;
			b2 = (1.0f - std::sqrt(2.0f) * K + K * K) * norm;
		} else {
			norm = 1.0f / (1.0f + std::sqrt(2.0f * V) * K + V * K * K);
			a0 = (1.0f + std::sqrt(2.0f) * K + K * K) * norm;
			a1 = 2.0f * (K * K - 1.0f) * norm;
			a2 = (1.0f - std::sqrt(2.0f) * K + K * K) * norm;
			b1 = 2.0f * (V * K * K - 1.0f) * norm;
			b2 = (1.0f - std::sqrt(2.0f * V) * K + V * K * K) * norm;
		}
		break;
	}

	return;
}

}
