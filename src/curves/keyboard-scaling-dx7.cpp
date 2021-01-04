/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <iostream>
#include "keyboard-scaling-dx7.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include "../imgui/imgui.h"
#include "../pling.hpp"
#include "../utils.hpp"

namespace Curve
{

float KeyboardScalingDX7::operator()(float freq) const
{
	float ratio = freq / breakpoint;

	if (ratio < 1.0f) {
		if (left_exponential) {
			return left_depth * (1.0f - ratio);
		} else {
			return left_depth * -std::log2(ratio);
		}
	} else {
		if (right_exponential) {
			return right_depth * (ratio - 1.0f);
		} else {
			return right_depth * std::log2(ratio);
		}
	}
}

void KeyboardScalingDX7::load(const YAML::Node &node)
{
	left_depth = node["left_depth"].as<float>(0);
	left_exponential = node["left_exponential"].as<bool>(false);
	right_depth = node["right_depth"].as<float>(0);
	right_exponential = node["right_exponential"].as<bool>(false);
	breakpoint = node["breakpoint"].as<float>(1);
}

YAML::Node KeyboardScalingDX7::save() const
{
	YAML::Node node;

	node["left_depth"] = left_depth;
	node["left_exponential"] = left_exponential;
	node["right_depth"] = right_depth;
	node["right_exponential"] = right_exponential;
	node["breakpoint"] = breakpoint;
	node.SetStyle(YAML::EmitterStyle::Flow);

	return node;
}

}
