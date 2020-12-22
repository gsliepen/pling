/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "octalope.hpp"

#include <cmath>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <iostream>

#include "pling.hpp"
#include "../imgui/imgui.h"
#include "../program-manager.hpp"
#include "utils.hpp"

bool Octalope::Voice::render(Chunk &chunk, Parameters &params)
{
	for (auto &sample : chunk.samples) {
		float accum{};
		float bend = params.bend * freq_envelope.update(params.freq_envelope);

		for (int i = 0; i < 8; ++i) {
			auto mod_source = params.ops[i].mod_source;
			float pm = mod_source < 8 ? ops[mod_source].value * params.ops[i].fm_level : 0.0f;
			ops[i].prev = ops[i].value;
			float value;

			switch (params.ops[i].waveform % 5) {
			case 0:
				value = ops[i].osc.fast_sine(pm);
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

			default:
				value = 0;
				break;
			}

			ops[i].value = ops[i].envelope.update(params.ops[i].envelope) * value;

			if (params.ops[i].am_level) {
				ops[i].value *= ops[mod_source].value;
			}

			if (params.ops[i].rm_level) {
				ops[i].value *= std::abs(ops[mod_source].value);
			}

			accum += ops[i].value * params.ops[i].output_level;
			ops[i].osc.update(delta * params.ops[i].freq_ratio * bend + params.ops[i].detune / sample_rate);
		}

		params.svf.set_freq(filter_envelope.update(params.filter_envelope) * freq);
		sample += svf(params.svf, accum * amp);
	}

	return is_active();
}

void Octalope::Voice::init(uint8_t key, float freq, float amp, const Parameters &params)
{
	this->freq = freq;
	this->delta = freq / sample_rate;
	this->amp = amp;
	freq_envelope.init(params.freq_envelope);
	filter_envelope.init(params.filter_envelope);

	for (int i = 0; i < 8; ++i) {
		ops[i].envelope.init(params.ops[i].envelope);
		ops[i].osc.init();
	}
}

void Octalope::Voice::release()
{
	freq_envelope.release();
	filter_envelope.release();

	for (auto &op : ops) {
		op.envelope.release();
	}
}

float Octalope::Voice::get_zero_crossing(float offset, const Parameters &params) const
{
	return ops[0].osc.get_zero_crossing(offset, delta * params.ops[0].freq_ratio * params.bend * freq_envelope.get());
}

float Octalope::Voice::get_frequency(const Parameters &params) const
{
	return freq * params.bend * freq_envelope.get();
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
	params.bend = exp2(value / 8192.0 / 6.0);
}

void Octalope::modulation(uint8_t value) {}
void Octalope::channel_pressure(int8_t pressure) {}
void Octalope::poly_pressure(uint8_t key, uint8_t pressure) {}

void Octalope::set_envelope(MIDI::Control control, uint8_t val, Envelope::ExponentialDX7::Parameters &envelope)
{
	int i = control.col % 4;

	switch (control.col) {
	case 0:
	case 1:
	case 2:
	case 3:
		envelope.level[i] = cc_linear(val, -24, 24);
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
	if (current_op_held) {
		if (current_op == 8) {
			set_envelope(control, val, params.freq_envelope);
		} else {
			set_envelope(control, val, params.ops[current_op].envelope);
		}
	} else if (current_op == 8 && control.col < 8) {
		set_envelope(control, val, params.filter_envelope);
	} else if (control.col < 8) {
		auto &op_params = params.ops[control.col];
		op_params.output_level = val ? dB_to_amplitude(cc_linear(val, -48, 0)) : 0;
		set_context(Context::MAIN);
	}
};

void Octalope::set_pot(MIDI::Control control, uint8_t val)
{
	if (current_op_held) {
		if (current_op < 8) {
			auto &op_params = params.ops[current_op];

			switch (control.col) {
			case 0:
				op_params.freq_ratio = val >= 64 ? val - 63 : 1.0f / (64 - val);
				set_context(Context::MAIN);
				break;

			case 1:
				op_params.detune = cc_linear(val, -10, 10);
				set_context(Context::MAIN);
				break;

			case 2:
				op_params.mod_source = val ? val / 16 : Operator::Parameters::NO_SOURCE;
				set_context(Context::MAIN);
				break;

			case 3:
				op_params.am_level = val;
				set_context(Context::MAIN);
				break;

			case 4:
				op_params.rm_level = val;
				set_context(Context::MAIN);
				break;

			case 5:
				op_params.waveform = val / 26;
				set_context(Context::MAIN);

			case 6:
			case 7:
			default:
				break;
			}
		}
	} else if (current_op == 8) {
		switch (control.col) {
		case 0:
			params.freq = cc_exponential(val, 0, 1, sample_rate / 6, sample_rate / 6);
			params.svf.set(params.svf_type, params.freq, params.Q);
			set_context(Context::MAIN);
			break;

		case 1:
			params.Q = cc_exponential(val, 1, 1, 1e2, 1e2);
			params.svf.set(params.svf_type, params.freq, params.Q);
			set_context(Context::MAIN);
			break;

		case 3:
			params.svf_type = static_cast<Filter::StateVariable::Parameters::Type>(cc_select(val, 4));
			params.svf.set(params.svf_type, params.freq, params.Q);
			set_context(Context::MAIN);
			break;

		default:
			break;
		}
	} else if (control.col < 8) {
		auto &op_params = params.ops[control.col];
		op_params.fm_level = val ? dB_to_amplitude(cc_linear(val, -48, 0)) : 0;
		set_context(Context::MAIN);
	}
}

void Octalope::set_button(MIDI::Control control, uint8_t val)
{
	if (val < 64) {
		return;
	}

	if (control.col < 8) {
		auto col = control.col + control.master * 8;

		if (current_op == col) {
			current_op_held ^= true;
		} else {
			current_op_held = true;
		}

		current_op = col;
		set_context(Context::MAIN);
	}
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
	// TODO: implement
	return true;
}

YAML::Node Octalope::save()
{
	YAML::Node yaml;
	// TODO: implement
	return yaml;
}

bool Octalope::build_main_widget()
{
	if (current_op_held) {
		if (current_op < 8) {
			const ImVec2 size{100, 100};
			ImGui::Begin(fmt::format("Octalope Op{} parameters", current_op + 1).c_str(), nullptr, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);
			ImGui::BeginTable("octalope-parameters", 6);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::VSliderFloat("##ratio", size, &params.ops[current_op].freq_ratio, 1.f / 64, 64.f);
			ImGui::TableSetColumnIndex(1);
			ImGui::VSliderFloat("##detune", size, &params.ops[current_op].detune, -10.f, 10.f);
			ImGui::TableSetColumnIndex(2);
			int src = params.ops[current_op].mod_source;
			src = src > 7 ? 0 : src + 1;
			ImGui::VSliderInt("##source", size, &src, 0, 8);
			ImGui::TableSetColumnIndex(3);
			ImGui::VSliderFloat("##AM", size, &params.ops[current_op].am_level, 0.f, 1.f);
			ImGui::TableSetColumnIndex(4);
			ImGui::VSliderFloat("##RM", size, &params.ops[current_op].rm_level, 0.f, 1.f);
			ImGui::TableSetColumnIndex(5);
			int waveform = params.ops[current_op].waveform + 1;
			ImGui::VSliderInt("##waveform", size, &waveform, 0, 7);
			ImGui::EndTable();
			ImGui::End();
		}
	} else {
		if (current_op < 8) {
			ImGui::Begin("Octalope global parameters", nullptr, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);

			ImGui::BeginTable("octalope-parameters", 9);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Op");

			for (int i = 0; i < 8; ++i) {
				ImGui::TableSetColumnIndex(i + 1);
				ImGui::Text(fmt::format("{}", i + 1).c_str());
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("Out");

			for (int i = 0; i < 8; ++i) {
				ImGui::TableSetColumnIndex(i + 1);
				ImGui::SliderFloat("", &params.ops[i].output_level, 0.f, 1.f);
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::Text("FM");

			for (int i = 0; i < 8; ++i) {
				ImGui::TableSetColumnIndex(i + 1);
				ImGui::SliderFloat("", &params.ops[i].fm_level, 1.f / 64, 64.f);
			}

			ImGui::EndTable();
			ImGui::End();
		} else {
			const ImVec2 size{100, 100};
			ImGui::Begin("Octalope filter parameters", nullptr, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);
			ImGui::BeginTable("octalope-parameters", 3);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::VSliderFloat("##freq", size, &params.freq, 0, 24000);
			ImGui::TableSetColumnIndex(1);
			ImGui::VSliderFloat("##res", size, &params.Q, 0.f, 1.f);
			ImGui::TableSetColumnIndex(2);
			int type = static_cast<int>(params.svf_type);
			ImGui::VSliderInt("##type", size, &type, 0, 7);
			ImGui::EndTable();
			ImGui::End();
		}
	}

	return true;
}

bool Octalope::build_context_widget()
{
	switch (get_context()) {
	case Context::MAIN:
		return build_main_widget();

	case Context::ENVELOPE:
		if (current_op < 8) {
			return params.ops[current_op].envelope.build_widget(fmt::format("Operator {}", int(current_op)));
		} else if (current_op_held) {
			return params.freq_envelope.build_widget("Frequency", 24.0f);
		} else {
			return params.filter_envelope.build_widget("Filter cutoff", 24.0f);
		}

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
