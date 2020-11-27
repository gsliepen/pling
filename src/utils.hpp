/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <cstdint>

constexpr float cc_linear(uint8_t val, float low, float high)
{
	return low + float(val) / 127.0f * (high - low);
}

constexpr float cc_linear(uint8_t val, float min, float low, float high, float max)
{
	if (val == 0) {
		return min;
	} else if (val == 127) {
		return max;
	}

	return min + float(val) / 127.0f * (max - min);
}

constexpr float cc_exponential(uint8_t val, float low, float high)
{
	return low * std::exp(val / 127.0f * std::log(high / low));
}

constexpr float cc_exponential(uint8_t val, float min, float low, float high, float max)
{
	if (val == 0) {
		return min;
	} else if (val == 127) {
		return max;
	}

	return low * std::exp(val / 127.0f * std::log(high / low));
}

constexpr uint8_t cc_select(uint8_t val, uint8_t max)
{
	return val / 128.0f * max;
}

constexpr float key_to_frequency(float key)
{
	return 440.0f * std::exp2((key - 69.0f) / 12.0f);
}

constexpr float amplitude_to_dB(float value)
{
	return 20.0f * std::log10(value);
}

constexpr float dB_to_amplitude(float value)
{
	return std::pow(10.0f, value / 20.0f);
}
