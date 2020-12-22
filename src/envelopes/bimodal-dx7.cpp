/* SPDX-License-Identifier: GPL-3.0-or-later */

#include <iostream>
#include "bimodal-dx7.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include "../imgui/imgui.h"
#include "../pling.hpp"
#include "../utils.hpp"

namespace Envelope
{

float BimodalDX7::update(const Parameters &param)
{
	switch (state) {
	case State::off:
		amplitude = param.level[0];
		break;

	case State::attack1:
	case State::attack2:
	case State::attack3: {
		int i = static_cast<int>(state);

		if (!param.duration[i - 1]) {
			amplitude = param.level[i];
			state = static_cast<State>(i + 1);
			break;
		}

		float rate = std::abs(param.level[i] - param.level[i - 1]) / (param.duration[i - 1] * sample_rate);

		if (amplitude > param.level[i]) {
			amplitude -= rate;

			if (amplitude <= param.level[i]) {
				amplitude = param.level[i];
				state = static_cast<State>(i + 1);
			}
		} else {
			amplitude += rate;

			if (amplitude >= param.level[i]) {
				amplitude = param.level[i];
				state = static_cast<State>(i + 1);
			}
		}

		break;
	}

	case State::sustain:
		amplitude = param.level[3];
		break;

	case State::release: {
		if (!param.duration[3]) {
			amplitude = param.level[0];
			state = State::off;
			break;
		}

		float rate = std::abs(param.level[0] - param.level[3]) / (param.duration[3] * sample_rate);

		if (amplitude > param.level[0]) {
			amplitude -= rate;

			if (amplitude <= param.level[0]) {
				amplitude = param.level[0];
				state = State::off;
			}
		} else {
			amplitude += rate;

			if (amplitude <= param.level[0]) {
				amplitude = param.level[0];
				state = State::off;
			}
		}

		break;
	}
	}

	return dB_to_amplitude(amplitude);
}

bool BimodalDX7::Parameters::build_widget(const std::string &name)
{
	ImGui::Begin((name + " envelope").c_str(), nullptr, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);

	/* Get the window position and size */
	const auto widget_pos = ImGui::GetCursorScreenPos();
	const auto region_min = ImGui::GetWindowContentRegionMin();
	const auto region_max = ImGui::GetWindowContentRegionMax();

	const auto x = widget_pos.x;
	const auto y = widget_pos.y;
	const auto w = region_max.x - region_min.x;
	const auto h = region_max.y - region_min.y;

	const float ct = y + h / 5.0f; /* curve top */
	const float cb = y + h; /* curve bottom */
	const float ch = cb - ct; /* curve height */
	const float pps = (w - 64.0f) / 10.0f; /* pixels per second */

	ImGui::BeginTable("##DX7", 8);

	for (int i = 0; i < 4; ++i) {
		float value = level[i];
		ImGui::TableNextColumn();
		ImGui::SliderFloat(fmt::format("L{}", i + 1).c_str(), &value, -48.0f, 0.0f, "%.1f dB");

		ImGui::TableNextColumn();
		value = duration[i];
		ImGui::SliderFloat(fmt::format("D{}", i + 1).c_str(), &value, 0.01f, 1.0f, "%.2f s");
	}

	ImGui::EndTable();

	auto list = ImGui::GetWindowDrawList();

	float heights[4];
	float widths[4];

	for (int i = 0; i < 4; ++i) {
		heights[i] = (1.0f + level[i] / 48.0f) * ch;
		widths[i] = pps * duration[i];
	}

	/* Draw the envelope shape */
	std::array<ImVec2, 6> coords;
	coords[0] = {x + 48, cb - heights[0]}; // attack1 start
	coords[1] = {coords[0].x + widths[0], cb - heights[1]}; // attack2 start
	coords[2] = {coords[1].x + widths[1], cb - heights[2]}; // attack3 start
	coords[3] = {coords[2].x + widths[2], cb - heights[3]}; // sustain start
	coords[4] = {coords[3].x + pps, cb - heights[3]}; // release start
	coords[5] = {coords[4].x + widths[3], cb - heights[0]}; // release end

	list->AddPolyline(coords.data(), coords.size(), ImColor(255, 255, 255, 255), false, 2);

	/* Draw transitions */
	static const char *labels[] = {"A1", "A2", "D", "S", "R"};

	for (int i = 0; i <= 5; ++i) {
		list->AddLine({coords[i].x, ct - 8}, {coords[i].x, cb}, ImColor{255, 255, 255, 128});

		if (i < 5 && coords[i + 1].x - coords[i].x > 8)
			list->AddText({0.5f * (coords[i].x + coords[i + 1].x) - 3, ct - 12}, ImColor(255, 255, 255, 128), labels[i]);
	}

	/* Draw grid lines */
	for (int i = 0; i <= 10; i++) {
		float xtick = coords[0].x + i * pps;
		auto label = fmt::format("{} s", i);
		list->AddLine({xtick, ct}, {xtick, cb}, ImColor(255, 255, 255, 64));
		list->AddText({xtick + 2, cb - 12}, ImColor(255, 255, 255, 128), label.c_str());
	}

	for (int i = 0; i < 4; i++) {
		float ytick = ct + ch * i / 4;
		auto label = fmt::format("{:3d} dB", i * -12);
		list->AddLine({x, ytick}, {x + w, ytick}, ImColor(255, 255, 255, 64));
		list->AddText({x + 2, ytick}, ImColor{255, 255, 255, 128}, label.c_str());
	}

	ImGui::End();

	return true;
}

}
