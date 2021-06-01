/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <functional>
#include <string>
#include <yaml-cpp/yaml.h>

#include "../pling.hpp"
#include "../utils.hpp"
#include "../imgui/imgui.h"

namespace Envelope
{

class ExponentialDX7
{
	float amplitude{};
	float duration{};
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

		void build_curve(float bimodal, ImColor color) const;
		bool build_widget(const std::string &name, float bimodal = 0.0f, std::function<void()> callback = [] {}) const;

		void load(const YAML::Node &yaml);
		YAML::Node save() const;
	};

	void reinit(const Parameters &param);

	void init(const Parameters &param)
	{
		amplitude = param.level[0];
		duration = param.duration[0];
		state = State::attack1;
	}

	bool is_active() const
	{
		return state != State::off;
	}

	void release(const Parameters &param)
	{
		state = State::release;
		duration = param.duration[3];
	}

	float update(const Parameters &param, float rate_scaling = 1.0f);

	float get() const
	{
		return dB_to_amplitude(amplitude);
	}
};

}
