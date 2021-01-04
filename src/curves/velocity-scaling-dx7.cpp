/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "velocity-scaling-dx7.hpp"

namespace Curve
{

float VelocityScalingDX7::operator()(float velocity) const
{
	float diff = velocity - breakpoint;

	if (diff < 0.0f) {
		if (left_exponential) {
			return std::exp2(-diff * left_depth);
		} else {
			return 1.0f - diff * left_depth;
		}
	} else {
		if (right_exponential) {
			return std::exp2(diff * right_depth);
		} else {
			return 1.0f + diff * right_depth;
		}
	}
};

void VelocityScalingDX7::load(const YAML::Node &node)
{
	left_depth = node["left_depth"].as<float>(0);
	left_exponential = node["left_exponential"].as<bool>(false);
	right_depth = node["right_depth"].as<float>(0);
	right_exponential = node["right_exponential"].as<bool>(false);
	breakpoint = node["breakpoint"].as<float>(1);
}

YAML::Node VelocityScalingDX7::save() const
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
