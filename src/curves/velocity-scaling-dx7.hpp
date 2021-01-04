/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cmath>
#include <string>

#include "../pling.hpp"
#include "../utils.hpp"
#include "../imgui/imgui.h"

namespace Curve
{

/**
 * Velocity scaling curve
 */
class VelocityScalingDX7
{
public:
	float breakpoint{1.0f}; // fraction
	bool left_exponential{};
	bool right_exponential{};
	float left_depth{}; // multiplier/unity range
	float right_depth{};

	bool build_widget(const std::string &name) const;

	void load(const YAML::Node &yaml);
	YAML::Node save() const;

	float operator()(float freq) const;
};

}
