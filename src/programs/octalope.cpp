/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "octalope.hpp"

#include <cmath>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>
#include <random>

#include "clock.hpp"
#include "pling.hpp"
#include "../imgui/imgui.h"
#include "../program-manager.hpp"
#include "utils.hpp"

static float rng(float range)
{
	std::uniform_real_distribution<float> dist{0.0f, range};
	return dist(random_engine);
}

void Octalope::Operator::Parameters::update_frequency()
{
	if (fixed) {
		frequency = std::exp2(frequency_coarse / 4 - 16) * (1.0f + frequency_fine / 120.0f);
	} else {
		float from = frequency_coarse >= 64 ? frequency_coarse - 63 : 1.0f / (65 - frequency_coarse);
		float to = frequency_coarse >= 63 ? frequency_coarse - 62 : 1.0f / (64 - frequency_coarse);
		frequency = from + (to - from) * frequency_fine / 120.0f;
	}
}

void Octalope::Operator::Parameters::set_detune(uint8_t val)
{
	detune = (val - 64) * 10.0f / 60.0f;
}

void Octalope::Operator::Parameters::set_flags(uint8_t val)
{
	val >>= 4;
	fixed = val & 1;
	sync = val & 2;
	tempo = val & 4;
	update_frequency();
}

void Octalope::Parameters::Filter::update_frequency()
{
	if (fixed) {
		frequency = std::exp2(frequency_coarse / 4 - 16) * (1.0f + frequency_fine / 120.0f);
	} else {
		float from = frequency_coarse >= 64 ? frequency_coarse - 63 : 1.0f / (65 - frequency_coarse);
		float to = frequency_coarse >= 63 ? frequency_coarse - 62 : 1.0f / (64 - frequency_coarse);
		frequency = from + (to - from) * frequency_fine / 120.0f;
	}
}

void Octalope::Parameters::Filter::set_flags(uint8_t val)
{
	val >>= 5;
	fixed = val & 1;
	tempo = val & 2;
	update_frequency();
}

bool Octalope::Voice::render(Chunk &chunk, Parameters &params)
{
	for (auto &sample : chunk.samples) {
		float accum{};

		// Determine the current voice frequency
		float voice_freq = frequency.base * std::exp2(params.bend * params.frequency.bend_sensitivity / 12.0f) * frequency.envelope.update(params.frequency.envelope, frequency.rate);

		if (params.frequency.lfo_depth || params.modulation) {
			voice_freq *= std::exp2((params.frequency.lfo_depth + params.frequency.mod_sensitivity * params.modulation) / 12.0f * ops[7].value);
		}

		// Go backwards through all operators,
		// since it's more likely that an operator is modulated by a higher number one,
		// and this way they are more likely to be exactly in sync.
		for (int i = 8; i--;) {
			// Determine the amount of phase modulation
			float pm{};

			for (int j = 0; j < 8; ++j) {
				pm += ops[j].value * params.ops[i].fm_level[j];
			}

			// Get the oscillator's output value
			float value;

			switch (params.ops[i].waveform % 5) {
			case 0:
				value = ops[i].osc.sine(pm);
				break;

			case 1:
				value = ops[i].osc.triangle(pm);
				break;

			case 2:
				value = ops[i].osc.square(pm);
				break;

			case 3:
				value = ops[i].osc.saw(pm);
				break;

			case 4:
				value = ops[i].osc.revsaw(pm);
				break;

			// TODO: S/H, noise, unity?

			default:
				value = 0;
				break;
			}

			// Apply amplitude modulations
			ops[i].value = ops[i].envelope.update(params.ops[i].envelope, ops[i].rate) * value * (ops[i].output_level);

			if (params.ops[i].am_level || params.modulation) {
				// assume ops[7].value has range -1..1
				ops[i].value *= 1.0f + (params.ops[i].am_level + params.modulation * params.ops[i].mod_sensitivity) * (ops[7].value - 1.0f) * 0.5;
			}

			accum += ops[i].value * params.ops[i].output_level;

			// Update the unmodulated phase of the oscillator
			float op_freq = params.ops[i].frequency;

			if (!params.ops[i].fixed) {
				op_freq *= voice_freq;
			}

			op_freq += params.ops[i].detune;
			ops[i].osc.update(op_freq / sample_rate);
		}

		// Apply the filter to the accumulated value so far
		float filter_freq = filter.base * params.filter.frequency;

		if (!params.filter.fixed) {
			filter_freq *= voice_freq;
		}

		if (params.filter.lfo_depth || params.filter.mod_sensitivity) {
			filter_freq *= std::exp2((params.filter.lfo_depth + params.modulation * params.filter.mod_sensitivity) / 12.0f * ops[7].value);
		}

		if (params.filter.bend_sensitivity) {
			filter_freq *= std::exp2(params.bend * params.filter.bend_sensitivity / 12.0f);
		}

		params.filter.svf.set_freq(filter.envelope.update(params.filter.envelope, filter.rate) * filter_freq);
		sample += filter.svf(params.filter.svf, accum);
	}

	return is_active();
}

void Octalope::Voice::init(uint8_t key, float freq, float velocity, const Parameters &params)
{
	frequency.base = freq * std::exp2(params.frequency.transpose / 12.0f) * std::exp2(rng(params.frequency.randomize / 12.0f));
	frequency.envelope.init(params.frequency.envelope);
	filter.base = std::exp2(rng(params.filter.randomize / 12.0f));
	filter.envelope.init(params.filter.envelope);

	for (int i = 0; i < 8; ++i) {
		auto keyboard_level = params.ops[i].keyboard_level_curve(freq);
		auto keyboard_rate = params.ops[i].keyboard_rate_curve(freq);
		auto velocity_level = params.ops[i].velocity_level_curve(velocity);
		auto velocity_rate = params.ops[i].velocity_rate_curve(velocity);

		ops[i].output_level = glm::clamp(velocity_level + keyboard_level, 0.0f, 1.0f);
		ops[i].rate = std::max(0.0f, velocity_rate + keyboard_rate);


		if (params.ops[i].sync) {
			ops[i].envelope.init(params.ops[i].envelope);
			ops[i].osc.init(params.ops[i].phase);
		} else {
			ops[i].envelope.reinit(params.ops[i].envelope);
			ops[i].osc.reinit(params.ops[i].phase);
		}

		if (params.ops[i].tempo) {
			ops[i].rate *= master_clock.get_tempo() / 120.0f;
		}
	}

	frequency.rate = 1;
	filter.rate = 1;

	if (params.frequency.tempo) {
		frequency.rate *= master_clock.get_tempo() / 120.0f;
	}

	if (params.filter.tempo) {
		filter.rate *= master_clock.get_tempo() / 120.0f;
	}
}

void Octalope::Voice::release()
{
	frequency.envelope.release();
	filter.envelope.release();

	for (auto &op : ops) {
		op.envelope.release();
	}
}

float Octalope::Voice::get_zero_crossing(float offset, const Parameters &params) const
{
	return ops[0].osc.get_zero_crossing(offset, frequency.base * std::exp2(params.bend * params.frequency.bend_sensitivity / 12.0f) * frequency.envelope.get() / sample_rate);
}

float Octalope::Voice::get_frequency(const Parameters &params) const
{
	return frequency.base * std::exp2(params.bend * params.frequency.bend_sensitivity / 12.0f) * frequency.envelope.get();
}

bool Octalope::render(Chunk &chunk)
{
	bool active = false;

	for (auto &voice : voices) {
		active |= voice.render(chunk, params);
	}

	return active;
}

float Octalope::get_zero_crossing(float offset) const
{
	float crossing = offset;

	if (auto lowest = voices.get_lowest(); lowest) {
		crossing = lowest->get_zero_crossing(offset, params);
	}

	return crossing;
}

float Octalope::get_base_frequency() const
{
	if (auto lowest = voices.get_lowest(); lowest) {
		return lowest->get_frequency(params);
	} else
		return {};
}

void Octalope::note_on(uint8_t key, uint8_t vel)
{
	Voice *voice = voices.press(key);

	if (!voice) {
		return;
	}

	float freq = 440.0 * std::exp2((key - 69) / 12.0);
	float amp = std::exp((vel - 127.) / 32.);
	voice->init(key, freq, amp, params);
}

void Octalope::note_off(uint8_t key, uint8_t vel)
{
	voices.release(key);
}

void Octalope::pitch_bend(int16_t value)
{
	params.bend = value / 8192.0;
}

void Octalope::modulation(uint8_t value)
{
	params.modulation = cc_linear(value, 0, 1);
}

void Octalope::channel_pressure(int8_t pressure) {}
void Octalope::poly_pressure(uint8_t key, uint8_t pressure) {}

void Octalope::set_envelope(MIDI::Control control, uint8_t val, Envelope::ExponentialDX7::Parameters &envelope, float from, float to)
{
	int i = control.col % 4;

	switch (control.col) {
	case 0:
	case 1:
	case 2:
	case 3:
		envelope.level[i] = cc_linear(val, from, from, to, to);
		set_context(Context::ENVELOPE);
		break;

	case 4:
	case 5:
	case 6:
	case 7:
		envelope.duration[i] = cc_exponential(val, 0, 1e-2, 1e1, 1e1);
		set_context(Context::ENVELOPE);
		break;

	default:
		break;
	}
}

void Octalope::set_fader(MIDI::Control control, uint8_t val)
{
	auto &op = params.ops[current_op];

	switch (current_page) {
	case Page::OPERATOR_WAVEFORM:
		set_envelope(control, val, op.envelope, -48.0f, 0.0f);
		set_context(Context::ENVELOPE);
		break;

	case Page::OPERATOR_MODULATION:
		op.fm_level[control.col] = cc_linear(val, 0, 0, 1, 1);
		set_context(Context::MAIN);
		break;

	case Page::OPERATOR_SCALING: {
		float depth = (val - 64.0f) * 0.1f;

		switch (control.col) {
		case 0:
			op.keyboard_level_curve.left_depth = depth;
			break;

		case 10:
			op.keyboard_level_curve.right_depth = depth;
			break;

		case 2:
			op.keyboard_rate_curve.left_depth = depth;
			break;

		case 3:
			op.keyboard_rate_curve.right_depth = depth;
			break;

		case 4:
			op.velocity_level_curve.left_depth = depth;
			break;

		case 5:
			op.velocity_level_curve.right_depth = depth;
			break;

		case 6:
			op.velocity_rate_curve.left_depth = depth;
			break;

		case 7:
			op.velocity_rate_curve.right_depth = depth;
			break;

		default:
			break;
		}

		set_context(Context::MAIN);
		break;
	}

	case Page::GLOBAL_PITCH:
		set_envelope(control, val, params.frequency.envelope, -24.0f, 24.0f);
		set_context(Context::ENVELOPE);
		break;

	case Page::GLOBAL_FILTER:
		set_envelope(control, val, params.filter.envelope, -24.0f, 24.0f);
		set_context(Context::ENVELOPE);
		break;
	}
};

void Octalope::set_pot(MIDI::Control control, uint8_t val)
{
	auto &op = params.ops[current_op];

	switch (current_page) {
	case Page::OPERATOR_WAVEFORM:
		switch (control.col) {
		case 0:
			op.set_frequency_coarse(val);
			break;

		case 1:
			op.set_frequency_fine(val);
			break;

		case 2:
			op.set_detune(val);
			break;

		case 3:
			op.set_flags(val);
			break;

		case 4:
			op.waveform = cc_select(val, 5);
			break;

		case 5:
			op.output_level = val ? cc_exponential(val, 0, 1.0f / 65536, 1, 1) : 0;
			break;

		case 6:
			op.phase = val / 128.0f;
			break;

		default:
			break;
		}

		break;

	case Page::OPERATOR_MODULATION:
		switch (control.col) {
		case 6:
			op.mod_sensitivity = cc_linear(val, 0, 2);
			break;

		case 7:
			op.am_level = cc_linear(val, 0, 2);
			break;

		default:
			break;
		}

		break;

	case Page::OPERATOR_SCALING:
		switch (control.col) {
		case 0:
			op.keyboard_level_curve.breakpoint = key_to_frequency(val);
			break;

		case 1:
			op.keyboard_level_curve.left_exponential = (val / 32) & 1;
			op.keyboard_level_curve.right_exponential = (val / 32) & 2;
			break;

		case 2:
			op.keyboard_rate_curve.breakpoint = key_to_frequency(val);
			break;

		case 3:
			op.keyboard_rate_curve.left_exponential = (val / 32) & 1;
			op.keyboard_rate_curve.right_exponential = (val / 32) & 2;
			break;

		case 4:
			op.velocity_level_curve.breakpoint = cc_linear(val, 0, 1);
			break;

		case 5:
			op.velocity_level_curve.left_exponential = (val / 32) & 1;
			op.velocity_level_curve.right_exponential = (val / 32) & 2;
			break;

		case 6:
			op.velocity_rate_curve.breakpoint = cc_linear(val, 0, 1);
			break;

		case 7:
			op.velocity_rate_curve.left_exponential = (val / 32) & 1;
			op.velocity_rate_curve.right_exponential = (val / 32) & 2;
			break;

		default:
			break;
		}

		break;

	case Page::GLOBAL_PITCH:
		switch (control.col) {
		case 0:
			params.frequency.transpose = val - 64;
			break;

		case 2:
			params.frequency.randomize = cc_exponential(val, 0, 0.01, 12, 12);
			break;

		case 3:
			params.frequency.tempo = val / 64;
			break;

		case 4:
			params.filter.bend_sensitivity = (val - 64) * 2.0f;
			break;

		case 5:
			params.frequency.bend_sensitivity = (val - 64) / 2.0f;
			break;

		case 6:
			params.frequency.mod_sensitivity = cc_linear(val, 0, 12);
			break;

		case 7:
			params.frequency.lfo_depth = cc_linear(val, 0, 12);
			break;

		default:
			break;
		}

		break;

	case Page::GLOBAL_FILTER:
		switch (control.col) {
		case 0:
			params.filter.set_frequency_coarse(val);
			break;

		case 1:
			params.filter.set_frequency_fine(val);
			break;

		case 2:
			params.filter.randomize = cc_exponential(val, 0, 0.1, 48, 48);
			break;

		case 3:
			params.filter.set_flags(val);
			break;

		case 4:
			params.filter.type = static_cast<Filter::StateVariable::Parameters::Type>(cc_select(val, 5));
			params.filter.svf.set(params.filter.type, params.filter.frequency, params.filter.Q);
			break;

		case 5:
			params.filter.Q = cc_exponential(val, 0, 1, 1e2, 1e2);
			params.filter.svf.set(params.filter.type, params.filter.frequency, params.filter.Q);
			break;

		case 6:
			params.filter.mod_sensitivity = cc_linear(val, 0, 48);
			break;

		case 7:
			params.filter.lfo_depth = cc_linear(val, 0, 48);
			break;

		default:
			break;
		}

		break;
	}

	set_context(Context::MAIN);
}

void Octalope::set_button(MIDI::Control control, uint8_t val)
{
	if (val < 64) {
		return;
	}

	if (control.master) {
		current_page = static_cast<Page>((static_cast<int>(current_page) + 1) % 5);
	} else if (control.col < 8) {
		current_op = control.col;
	}

	set_context(get_context() != Context::NONE ? get_context() : Context::MAIN);
}

void Octalope::sustain(bool val)
{
	voices.set_sustain(val);
}

void Octalope::release_all()
{
	voices.release_all();
}

bool Octalope::load(const YAML::Node &yaml)
{
	int i = 0;

	for (auto &node : yaml["operators"]) {
		auto &op = params.ops[i++];
		op.frequency = node["frequency"].as<float>(1);
		op.detune = node["detune"].as<float>(0);
		op.phase = node["phase"].as<float>(0);
		op.output_level = node["output_level"].as<float>(0);
		op.waveform = node["waveform"].as<int>(0);
		op.fixed = node["fixed_frequency"].as<bool>(false);
		op.sync = node["sync_start"].as<bool>(false);
		op.tempo = node["tempo_sync"].as<bool>(false);
		int j = 0;

		for (auto &fm : node["fm_level"]) {
			op.fm_level[j++] = fm.as<float>(0);
		}

		op.mod_sensitivity = node["mod_sensitivity"].as<float>(0);
		op.am_level = node["am_level"].as<float>(0);

		op.envelope.load(node["envelope"]);
		op.keyboard_level_curve.load(node["keyboard_level_curve"]);
		op.keyboard_rate_curve.load(node["keyboard_rate_curve"]);
		op.velocity_level_curve.load(node["velocity_level_curve"]);
		op.velocity_rate_curve.load(node["velocity_rate_curve"]);
	}

	{
		auto &node = yaml["frequency"];
		params.frequency.transpose = node["transpose"].as<float>(0);
		params.frequency.randomize = node["randomize"].as<float>(0);
		params.frequency.bend_sensitivity = node["bend_sensitivity"].as<float>(2);
		params.frequency.mod_sensitivity = node["mod_sensitivity"].as<float>(0);
		params.frequency.lfo_depth = node["lfo_depth"].as<float>(0);
		params.frequency.tempo = node["tempo_sync"].as<bool>(false);
		params.frequency.envelope.load(node["envelope"]);
	}

	{
		auto &node = yaml["filter"];
		params.filter.frequency = node["frequency"].as<float>(0);
		params.filter.randomize = node["randomize"].as<float>(0);
		params.filter.type = static_cast<Filter::StateVariable::Parameters::Type>(node["type"].as<int>(0));
		params.filter.Q = node["Q"].as<float>(0);
		params.filter.svf.set(params.filter.type, params.filter.frequency, params.filter.Q);
		params.filter.bend_sensitivity = node["bend_sensitivity"].as<float>(0);
		params.filter.mod_sensitivity = node["mod_sensitivity"].as<float>(0);
		params.filter.lfo_depth = node["lfo_depth"].as<float>(0);
		params.filter.fixed = node["fixed_frequency"].as<bool>(false);
		params.filter.tempo = node["tempo_sync"].as<bool>(false);
		params.filter.envelope.load(node["envelope"]);
	}

	return true;
}

YAML::Node Octalope::save()
{
	YAML::Node yaml;

	for (int i = 0; i < 8; ++i) {
		YAML::Node node;
		auto &op = params.ops[i];
		node["frequency"] = op.frequency;
		node["detune"] = op.detune;
		node["phase"] = op.phase;
		node["output_level"] = op.output_level;
		node["waveform"] = int(op.waveform);
		node["fixed_frequency"] = op.fixed;
		node["sync_start"] = op.sync;
		node["tempo_sync"] = op.tempo;

		for (auto fm : op.fm_level) {
			node["fm_level"].push_back(fm);
		}

		node["fm_level"].SetStyle(YAML::EmitterStyle::Flow);
		node["mod_sensitivity"] = op.mod_sensitivity;
		node["am_level"] = op.am_level;

		node["envelope"] = op.envelope.save();
		node["keyboard_level_curve"] = op.keyboard_level_curve.save();
		node["keyboard_rate_curve"] = op.keyboard_rate_curve.save();
		node["velocity_level_curve"] = op.velocity_level_curve.save();
		node["velocity_rate_curve"] = op.velocity_rate_curve.save();

		yaml["operators"].push_back(node);
	}

	{
		YAML::Node node;
		node["transpose"] = params.frequency.transpose;
		node["randomize"] = params.frequency.randomize;
		node["bend_sensitivity"] = params.frequency.bend_sensitivity;
		node["mod_sensitivity"] = params.frequency.mod_sensitivity;
		node["lfo_depth"] = params.frequency.lfo_depth;
		node["tempo_sync"] = params.frequency.tempo;
		node["envelope"] = params.frequency.envelope.save();
		yaml["frequency"] = node;
	}

	{
		YAML::Node node;
		node["frequency"] = params.filter.frequency;
		node["randomize"] = params.filter.randomize;
		node["type"] = static_cast<int>(params.filter.type);
		node["Q"] = params.filter.Q;
		node["bend_sensitivity"] = params.filter.bend_sensitivity;
		node["mod_sensitivity"] = params.filter.mod_sensitivity;
		node["lfo_depth"] = params.filter.lfo_depth;
		node["fixed_frequency"] = params.filter.fixed;
		node["tempo_sync"] = params.filter.tempo;
		node["envelope"] = params.filter.envelope.save();
		yaml["filter"] = node;
	}

	return yaml;
}

/*  Colorbrewer2 8 color "paired" */
static const ImColor colors[] = {
	ImColor(166, 206, 227, 128),
	ImColor(31, 120, 180, 128),
	ImColor(178, 223, 138, 128),
	ImColor(51, 160, 44, 128),
	ImColor(251, 154, 153, 128),
	ImColor(227, 26, 28, 128),
	ImColor(253, 191, 111, 128),
	ImColor(255, 127, 0, 128),
};

bool Octalope::build_operator_waveform_widget()
{
	auto &op = params.ops[current_op];

	switch (get_context()) {
	case Context::MAIN: {
		ImGui::Begin(fmt::format("Operator {} waveform", current_op + 1).c_str(), {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);

		if (op.fixed) {
			ImGui::InputFloat("Frequency", &op.frequency, 0.0f, 10000.0f);
		} else {
			ImGui::InputFloat("Ratio", &op.frequency, 0.0f, 64.0f);
		}

		ImGui::InputFloat("Detune", &op.detune, -10.f, 10.f);
		ImGui::Checkbox("Fixed frequency", &op.fixed);
		ImGui::SameLine();
		ImGui::Checkbox("Sync start", &op.sync);
		ImGui::SameLine();
		ImGui::Checkbox("Tempo sync", &op.tempo);
		static const char *waveform_names[] = {"Sine", "Triangle", "Square", "Saw", "Rev. Saw"};
		int waveform = op.waveform;
		ImGui::SliderInt("Waveform", &waveform, 0, 4, waveform_names[op.waveform]);
		ImGui::InputFloat("Output level", &op.output_level, 0, 1.0f);
		ImGui::InputFloat("Phase", &op.phase, 0.0f, 1.0f);
		ImGui::End();
		return true;
	}

	case Context::ENVELOPE:
		return op.envelope.build_widget(fmt::format("Operator {}", current_op + 1), 0.0f, [&] {
			for (int i = 0; i < 8; i++)
			{
				if (i != current_op) {
					params.ops[i].envelope.build_curve(0.0f, colors[i]);
				}
			}
		});

	default:
		return false;
	}
}

bool Octalope::build_operator_modulation_widget()
{
	auto &op = params.ops[current_op];

	switch (get_context()) {
	case Context::MAIN:
	case Context::ENVELOPE: {
		ImGui::Begin(fmt::format("Operator {} modulation", current_op + 1).c_str(), {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);
		ImGui::InputFloat("Mod sensitivity", &op.mod_sensitivity, 0.0f, 2.0f);
		ImGui::InputFloat("Op8 mod depth", &op.am_level, 0.0f, 2.0f);
		ImGui::BeginTable("operator-modulation", 2);

		for (int i = 0; i < 8; i++) {
			if ((i % 2) == 0) {
				ImGui::TableNextRow();
			}

			ImGui::TableNextColumn();
			ImGui::InputFloat(fmt::format("Op{} FM level", i + 1).c_str(), &op.fm_level[i], 0.0f, 1.0f);
		}

		ImGui::EndTable();
		ImGui::End();
		return true;
	}

	default:
		return false;
	}
}

bool Octalope::build_operator_scaling_widget()
{
	auto &op = params.ops[current_op];

	switch (get_context()) {
	case Context::MAIN:
	case Context::ENVELOPE: {
		ImGui::Begin(fmt::format("Operator {} scaling", current_op + 1).c_str(), {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);
		ImGui::BeginTable("operator-scaling", 4);
		ImGui::TableNextColumn();
		ImGui::Text("Keyboard level");
		ImGui::InputFloat("Left", &op.keyboard_level_curve.left_depth, -1.0f, 1.0f);
		ImGui::InputFloat("Break", &op.keyboard_level_curve.breakpoint, 1.0f, 10000.0f);
		ImGui::InputFloat("Right", &op.keyboard_level_curve.right_depth, -1.0f, 1.0f);
		ImGui::TableNextColumn();
		ImGui::Text("Keyboard rate");
		ImGui::InputFloat("Left", &op.keyboard_rate_curve.left_depth, -1.0f, 1.0f);
		ImGui::InputFloat("Break", &op.keyboard_rate_curve.breakpoint, 1.0f, 10000.0f);
		ImGui::InputFloat("Right", &op.keyboard_rate_curve.right_depth, -1.0f, 1.0f);
		ImGui::TableNextColumn();
		ImGui::Text("Velocity level");
		ImGui::InputFloat("Left", &op.velocity_level_curve.left_depth, -1.0f, 1.0f);
		ImGui::InputFloat("Break", &op.velocity_level_curve.breakpoint, 0.0f, 1.0f);
		ImGui::InputFloat("Right", &op.velocity_level_curve.right_depth, -1.0f, 1.0f);
		ImGui::TableNextColumn();
		ImGui::Text("Velocity rate");
		ImGui::InputFloat("Left", &op.velocity_rate_curve.left_depth, -1.0f, 1.0f);
		ImGui::InputFloat("Break", &op.velocity_rate_curve.breakpoint, 0.0f, 1.0f);
		ImGui::InputFloat("Right", &op.velocity_rate_curve.right_depth, -1.0f, 1.0f);
		ImGui::EndTable();
		ImGui::End();
		return true;
	}

	default:
		return false;
	}
}

bool Octalope::build_global_pitch_widget()
{
	switch (get_context()) {
	case Context::MAIN: {
		ImGui::Begin("Global pitch", {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);
		ImGui::InputFloat("Transpose", &params.frequency.transpose, 0.0f, 16.0f);
		ImGui::InputFloat("Randomize", &params.frequency.randomize, 0.0f, 12.0f);
		ImGui::Checkbox("Tempo sync", &params.frequency.tempo);
		ImGui::InputFloat("Filter bend sensitivity", &params.filter.bend_sensitivity, 0, 2.0f);
		ImGui::InputFloat("Bend sensitivity", &params.frequency.bend_sensitivity, 0, 2.0f);
		ImGui::InputFloat("Mod sensitivity", &params.frequency.mod_sensitivity, 0, 2.0f);
		ImGui::InputFloat("Op8 mod depth", &params.frequency.lfo_depth, 0.0f, 2.0f);
		ImGui::End();
		return true;
	}

	case Context::ENVELOPE:
		return params.frequency.envelope.build_widget(fmt::format("Pitch", int(current_op)));

	default:
		return false;
	}
}

bool Octalope::build_global_filter_widget()
{
	switch (get_context()) {
	case Context::MAIN: {
		ImGui::Begin("Global filter", {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);

		//ImGui::InputFloat("Frequency", &params.filter.frequency, 0.0f, 10000.0f);
		if (params.filter.fixed) {
			ImGui::InputFloat("Ratio", &params.filter.frequency, 0.0f, 64.0f);
		} else {
			ImGui::InputFloat("Frequency", &params.filter.frequency, 0.0f, 65536.0f);
		}

		ImGui::InputFloat("Randomize", &params.filter.randomize, 0.0f, 12.0f);
		ImGui::Checkbox("Fixed frequency", &params.filter.fixed);
		ImGui::SameLine();
		ImGui::Checkbox("Tempo sync", &params.filter.tempo);
		static const char *type_names[] = {"Off", "12 dB Low pass", "12 dB High pass", "12 dB Band pass", "12 dB Notch"};
		int type = static_cast<int>(params.filter.type);
		ImGui::SliderInt("Type", &type, 0, 4, type_names[type]);
		ImGui::InputFloat("Q", &params.filter.Q, 0, 100.0f);
		ImGui::InputFloat("Mod sensitivity", &params.filter.mod_sensitivity, 0, 2.0f);
		ImGui::InputFloat("Op8 mod depth", &params.filter.lfo_depth, 0.0f, 16.0f);
		ImGui::End();
		return true;
	}

	case Context::ENVELOPE:
		return params.filter.envelope.build_widget(fmt::format("Filter cutoff", int(current_op)));

	default:
		return false;
	}
}

bool Octalope::build_context_widget()
{
	if (get_context() == Context::NONE) {
		return false;
	}

	switch (current_page) {
	case Page::OPERATOR_WAVEFORM:
		return build_operator_waveform_widget();

	case Page::OPERATOR_MODULATION:
		return build_operator_modulation_widget();

	case Page::OPERATOR_SCALING:
		return build_operator_scaling_widget();

	case Page::GLOBAL_PITCH:
		return build_global_pitch_widget();

	case Page::GLOBAL_FILTER:
		return build_global_filter_widget();

	default:
		return false;
	}
}

static const std::string engine_name{"Octalope"};

const std::string &Octalope::get_engine_name()
{
	return engine_name;
}

static auto registration = programs.register_engine(engine_name, []()
{
	return std::make_shared<Octalope>();
});
