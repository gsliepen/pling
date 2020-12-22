/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <random>

#include "../pling.hpp"

namespace Oscillator
{

class PM
{
private:
	float phase{};

public:
	PM() = default;

	void init()
	{
		this->phase = {};
	}

	void update(float delta)
	{
		phase += delta;
		phase -= std::floor(phase);
	};

	float update_sync(float delta)
	{
		float prev = phase;
		phase += delta;
		phase -= std::floor(phase);
		return phase - prev;
	};

	constexpr float sine(float pm) const
	{
		return std::sin((phase + pm) * float(2 * M_PI));
	}

	constexpr float fast_sine(float pm) const
	{
		// Approximation of a sine using a parabola, without using branches.
		const float x1 = frac(pm) - 0.5f;
		const float x2 = std::abs(x1) * 4.0f - 1.0f;
		const float v = 1.0f - x2 * x2;
		return std::copysign(v, x1);
	}

	constexpr float square(float pm) const
	{
		return frac(pm) < 0.5f ? 1.0f : -1.0f;
	}

	constexpr float triangle(float pm) const
	{
		return std::abs(frac(pm - 0.25f) - 0.5f) * 4.0f - 1.0f;
	}

	constexpr float saw(float pm) const
	{
		return frac(pm) * -2.0f + 1.0f;
	}

	constexpr float revsaw(float pm) const
	{
		return frac(pm) * 2.0f - 1.0f;
	}

	constexpr float operator()(float pm) const
	{
		return frac(pm);
	}

	operator float() const
	{
		return phase;
	}

	constexpr float operator()() const
	{
		return phase;
	}

	constexpr float frac(float pm) const
	{
		float value = phase + pm;
		return value - std::floor(value);
	}

	constexpr float get_zero_crossing(float offset, float delta) const
	{
		/* Going backwards from the current sample position + offset,
		 * return the first zero crossing of the phase of this oscillator. */

		float phase_at_offset = phase + offset * delta;
		phase_at_offset -= std::floor(phase_at_offset);
		return offset - phase_at_offset / delta;
	}
};

}
