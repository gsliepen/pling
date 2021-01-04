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
 * Keyboard scaling curve, modeled after the DX7.
 *
 * This is not an exact replica of the DX7 keyboard scaling, but it has the same feature set:
 * - A configurable breakpoint (given by frequency in this case)
 * - Left and right curve configurable separately
 * - Linear curve: curve depth * distance in octaves from breakpoint
 * - Exponential curve: curve depth * doubling every octave, such that 6 octaves gives exactly curve depth
 */
class KeyboardScalingDX7
{
public:
	float breakpoint{440.0f}; // Hz
	bool left_exponential{};
	bool right_exponential{};
	float left_depth{}; // multiplier/octave
	float right_depth{};

	bool build_widget(const std::string &name) const;

	void load(const YAML::Node &yaml);
	YAML::Node save() const;

	float operator()(float freq) const;
};

}
