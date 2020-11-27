/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <vector>

#include "config.hpp"

extern float sample_rate;
static const size_t chunk_size = 128;

extern Config config;

struct Chunk {
	std::array<float, chunk_size> samples;

	void clear()
	{
		samples.fill({});
	}
};

class RingBuffer
{
	size_t pos{};
	std::atomic<size_t> tail{};
	std::atomic<float> best_crossing{};
	std::atomic<float> base_frequency{};
	std::atomic<float> avg_rms = 0;
	std::vector<float> samples;

public:
	RingBuffer(size_t size): samples(size)
	{
		assert(size % chunk_size == 0);
	}

	void add(const Chunk &chunk, float zero_crossing = 0, float frequency = 0)
	{
		float rms = 0;

		for (size_t i = 0; i < chunk.samples.size(); ++i) {
			rms += chunk.samples[i] * chunk.samples[i];
			samples[pos++] = chunk.samples[i];
		}

		best_crossing = zero_crossing + pos;
		base_frequency = frequency;
		pos %= samples.size();
		tail = pos;
		rms = sqrtf(rms) / 8.0f;
		avg_rms = avg_rms * 0.95f + rms * 0.05f;
	}

	float get_crossing() const
	{
		return best_crossing;
	}

	float get_rms() const
	{
		return avg_rms;
	}

	float get_base_frequency() const
	{
		return base_frequency;
	}

	const std::vector<float> &get_samples() const
	{
		return samples;
	}

	const size_t get_tail() const
	{
		return tail;
	}
};

