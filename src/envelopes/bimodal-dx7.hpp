/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <string>

#include "../pling.hpp"

namespace Envelope
{

class BimodalDX7
{
	float amplitude{};
	enum class State {
		off,
		attack1,
		attack2,
		attack3,
		sustain,
		release,
	} state;

public:
	struct Parameters {
		float level[4] {};
		float duration[4] {};

		bool build_widget(const std::string &name);
	};

	void init(const Parameters &param)
	{
		amplitude = param.level[0];
		state = State::attack1;
	}

	bool is_active() const
	{
		return state != State::off;
	}

	void release()
	{
		state = State::release;
	}

	float update(const Parameters &param);
};

}
