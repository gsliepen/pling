/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "exponential-adsr.hpp"

#include <fmt/format.h>
#include <glm/glm.hpp>
#include "../imgui/imgui.h"
#include "../pling.hpp"
#include "../utils.hpp"

namespace Envelope {

float ExponentialADSR::update(Parameters &param) {
	switch(state) {
	case State::off:
		amplitude = 0;
		break;

	case State::attack:
		amplitude += param.attack;
		if (amplitude >= 1) {
			amplitude = 1;
			state = State::decay;
		}
		break;

	case State::decay:
		amplitude = param.sustain + (amplitude - param.sustain) * param.decay;
		break;

	case State::release:
		amplitude *= param.release;
		if (amplitude < cutoff) {
			amplitude = 0;
			state = State::off;
		}
		break;
	}

	return amplitude;
}

bool ExponentialADSR::Parameters::build_widget(const std::string &name) {
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

	float attack_time = get_attack();
	float decay_time = get_decay();
	float sustain_level = amplitude_to_dB(get_sustain());
	float release_time = get_release();

	float attack_width = attack_time * pps;
	float decay_width = decay_time * pps;
	float sustain_width = pps;
	float sustain_height = (1.0f + sustain_level / 48.0f) * ch;
	float release_width = release_time * pps;

	ImGui::Columns(4);
	if (ImGui::InputFloat("Attack", &attack_time, 0.01f, 1.0f, "%.2f s"))
		set_attack(attack_time);
	ImGui::NextColumn();
	if (ImGui::InputFloat("Decay", &decay_time, 0.01f, 1.0f, "%.2f s"))
		set_decay(decay_time);
	ImGui::NextColumn();
	if (ImGui::InputFloat("Sustain", &sustain_level, 0.1f, 1.0f, "%.1f dB"))
		set_sustain(dB_to_amplitude(sustain_level));
	ImGui::NextColumn();
	if (ImGui::InputFloat("Release", &release_time, 0.01f, 1.0f, "%.2f s"))
		set_release(release_time);
	ImGui::Columns();

	auto list = ImGui::GetWindowDrawList();

	/* Draw the envelope shape */
	std::array<ImVec2, 5> coords;
	coords[0] = {x + 48, cb}; // attack start
	coords[1] = {coords[0].x + attack_width, cb - ch}; // attack end, decay start
	coords[2] = {coords[1].x + decay_width, cb - sustain_height}; // decay end, sustain start
	coords[3] = {coords[2].x + sustain_width, cb - sustain_height}; // sustain end, release start
	coords[4] = {coords[3].x + release_width, cb}; // release end

	list->AddPolyline(coords.data(), coords.size(), ImColor(255, 255, 255, 255), false, 2);

	/* Draw transitions */
	static const char *ADSR[] = {"A", "D", "S", "R"};

	for (int i = 0; i <= 4; ++i) {
		list->AddLine({coords[i].x, ct - 8}, {coords[i].x, cb}, ImColor{255, 255, 255, 128});
		if (i < 4 && coords[i + 1].x - coords[i].x > 8)
			list->AddText({0.5f * (coords[i].x + coords[i + 1].x) - 3, ct - 12}, ImColor(255, 255, 255, 128), ADSR[i]);
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
