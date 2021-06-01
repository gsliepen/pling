/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <cstdint>

static inline float cc_linear(uint8_t val, float low, float high)
{
	return low + float(val) / 127.0f * (high - low);
}

static inline float cc_linear(uint8_t val, float min, float low, float high, float max)
{
	if (val == 0) {
		return min;
	} else if (val == 127) {
		return max;
	}

	return min + float(val) / 127.0f * (max - min);
}

static inline float cc_exponential(uint8_t val, float low, float high)
{
	return low * std::exp(val / 127.0f * std::log(high / low));
}

static inline float cc_exponential(uint8_t val, float min, float low, float high, float max)
{
	if (val == 0) {
		return min;
	} else if (val == 127) {
		return max;
	}

	return low * std::exp(val / 127.0f * std::log(high / low));
}

static inline uint8_t cc_select(uint8_t val, uint8_t max)
{
	return val / 128.0f * max;
}

static inline float key_to_frequency(float key)
{
	return 440.0f * std::exp2((key - 69.0f) / 12.0f);
}

static inline float amplitude_to_dB(float value)
{
	return 20.0f * std::log10(value);
}

static inline float dB_to_amplitude(float value)
{
	return std::pow(10.0f, value / 20.0f);
}
