/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <vector>

static const float sample_rate = 48000;
static const size_t chunk_size = 128;

struct Chunk {
	std::array<float, chunk_size> samples;

	void clear() {
		samples.fill({});
	}
};

class RingBuffer {
	size_t pos{};
	std::atomic<size_t> tail{};
	std::atomic<float> best_crossing{};
	std::vector<float> samples;

	public:
	RingBuffer(size_t size): samples(size) {
		assert(size % chunk_size == 0);
	}

	void add(const Chunk &chunk, float zero_crossing = 0) {
		std::copy(chunk.samples.begin(), chunk.samples.end(), &samples[pos]);
		pos += chunk_size;
		best_crossing = zero_crossing + pos;
		pos %= samples.size();
		tail = pos;
	}

	float get_crossing() const {
		return best_crossing;
	}

	const std::vector<float> &get_samples() const {
		return samples;
	}

	const size_t get_tail() const {
		return tail;
	}
};

