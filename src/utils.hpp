/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <cstdint>

inline float cc_linear(uint8_t val, float low, float high) {
	return low + float(val) / 127.0f * (high - low);
}

inline float cc_linear(uint8_t val, float min, float low, float high, float max) {
	if (val == 0)
		return min;
	else if (val == 127)
		return max;

	return min + float(val) / 127.0f * (max - min);
}

inline float cc_exponential(uint8_t val, float low, float high) {
	return low * expf(val / 127.0f * logf(high / low));
}

inline float cc_exponential(uint8_t val, float min, float low, float high, float max) {
	if (val == 0)
		return min;
	else if (val == 127)
		return max;

	return low * expf(val / 127.0f * logf(high / low));
}

inline uint8_t cc_select(uint8_t val, uint8_t max) {
	return val / 128.0f * max;
}
