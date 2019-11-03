/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstddef>
#include <array>

static const float sample_rate = 48000;
static const size_t chunk_size = 128;

struct Chunk {
	std::array<float, chunk_size> samples;

	void clear() {
		samples.fill({});
	}
};
