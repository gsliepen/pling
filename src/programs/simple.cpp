/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "simple.hpp"

#include <cmath>
#include <fmt/ostream.h>
#include <iostream>

#include "pling.hpp"
#include "../program-manager.hpp"
#include "utils.hpp"

bool Simple::Voice::render(Chunk &chunk, Parameters &params)
{
	for (auto &sample : chunk.samples) {
		params.svf.set_freq(filter_envelope.update(params.filter_envelope) * params.freq);
		sample += svf(params.svf, osc.saw() * amp * amplitude_envelope.update(params.amplitude_envelope) * (1 - (lfo.fast_sine() * 0.5 + 0.5) * params.mod));
		++lfo;
		osc.update(params.bend);
	}

	return is_active();
}

void Simple::Voice::init(uint8_t key, float freq, float amp)
{
	lfo.init(10);
	osc.init(freq);
	this->amp = amp;
	amplitude_envelope.init();
	filter_envelope.init();
}

void Simple::Voice::release()
{
	amplitude_envelope.release();
	filter_envelope.release();
}

float Simple::Voice::get_zero_crossing(float offset, const Parameters &params) const
{
	return osc.get_zero_crossing(offset, params.bend);
}

float Simple::Voice::get_frequency(const Parameters &params) const
{
	return osc.get_frequency(params.bend);
}

bool Simple::render(Chunk &chunk)
{
	bool active = false;

	for (auto &voice : voices) {
		active |= voice.render(chunk, params);
	}

	return active;
}

float Simple::get_zero_crossing(float offset) const
{
	float crossing = offset;

	if (auto lowest = voices.get_lowest(); lowest) {
		crossing = lowest->get_zero_crossing(offset, params);
	}

	return crossing;
}

float Simple::get_base_frequency() const
{
	if (auto lowest = voices.get_lowest(); lowest) {
		return lowest->get_frequency(params);
	} else
		return {};
}

void Simple::note_on(uint8_t key, uint8_t vel)
{
	Voice *voice = voices.press(key);

	if (!voice) {
		return;
	}

	float freq = 440.0 * std::exp2((key - 69) / 12.0);
	float amp = std::exp((vel - 127.) / 32.);
	voice->init(key, freq, amp);
}

void Simple::note_off(uint8_t key, uint8_t vel)
{
	voices.release(key);
}

void Simple::pitch_bend(int16_t value)
{
	params.bend = exp2(value / 8192.0 / 6.0);
}

void Simple::modulation(uint8_t value)
{
	params.mod = cc_linear(value, 0, 1);
}

void Simple::channel_pressure(int8_t pressure) {}
void Simple::poly_pressure(uint8_t key, uint8_t pressure) {}

void Simple::set_fader(MIDI::Control control, uint8_t val)
{
	switch (control.col) {
	case 0:
		params.amplitude_envelope.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		set_context(Context::AMPLITUDE_ENVELOPE);
		break;

	case 1:
		params.amplitude_envelope.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		set_context(Context::AMPLITUDE_ENVELOPE);
		break;

	case 2:
		params.amplitude_envelope.set_sustain(dB_to_amplitude(cc_linear(val, -48, 0)));
		set_context(Context::AMPLITUDE_ENVELOPE);
		break;

	case 3:
		params.amplitude_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		set_context(Context::AMPLITUDE_ENVELOPE);
		break;

	case 4:
		params.filter_envelope.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		set_context(Context::FILTER_ENVELOPE);
		break;

	case 5:
		params.filter_envelope.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		set_context(Context::FILTER_ENVELOPE);
		break;

	case 6:
		params.filter_envelope.set_sustain(dB_to_amplitude(cc_linear(val, -48, 0)));
		set_context(Context::FILTER_ENVELOPE);
		break;

	case 7:
		params.filter_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		set_context(Context::FILTER_ENVELOPE);
		break;

	default:
		break;
	}
};

void Simple::set_pot(MIDI::Control control, uint8_t val)
{
	switch (control.col) {
	case 0:
		params.freq = cc_exponential(val, 0, 1, sample_rate / 6, sample_rate / 6);
		params.svf.set(params.svf_type, params.freq, params.Q);
		break;

	case 1:
		params.Q = cc_exponential(val, 1, 1, 1e2, 1e2);
		params.svf.set(params.svf_type, params.freq, params.Q);
		break;

	case 3:
		params.svf_type = static_cast<Filter::StateVariable::Parameters::Type>(cc_select(val, 4));
		params.svf.set(params.svf_type, params.freq, params.Q);
		break;
	}
}

void Simple::set_button(MIDI::Control control, uint8_t val)
{
}

void Simple::sustain(bool val)
{
	voices.set_sustain(val);
}

void Simple::release_all()
{
	voices.release_all();
}

bool Simple::load(const YAML::Node &yaml)
{
	params.amplitude_envelope.set_attack(yaml["amplitude_envelope"][0].as<float>());
	params.amplitude_envelope.set_decay(yaml["amplitude_envelope"][1].as<float>());
	params.amplitude_envelope.set_sustain(yaml["amplitude_envelope"][2].as<float>());
	params.amplitude_envelope.set_release(yaml["amplitude_envelope"][3].as<float>());

	params.filter_envelope.set_attack(yaml["filter_envelope"][0].as<float>());
	params.filter_envelope.set_decay(yaml["filter_envelope"][1].as<float>());
	params.filter_envelope.set_sustain(yaml["filter_envelope"][2].as<float>());
	params.filter_envelope.set_release(yaml["filter_envelope"][3].as<float>());

	params.freq = yaml["filter"][0].as<float>();
	params.Q = yaml["filter"][0].as<float>();
	params.gain = yaml["filter"][0].as<float>();

	return true;
}

YAML::Node Simple::save()
{
	YAML::Node yaml;

	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.attack);
	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.decay);
	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.sustain);
	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.release);

	yaml["filter_envelope"].push_back(params.filter_envelope.attack);
	yaml["filter_envelope"].push_back(params.filter_envelope.decay);
	yaml["filter_envelope"].push_back(params.filter_envelope.sustain);
	yaml["filter_envelope"].push_back(params.filter_envelope.release);

	yaml["filter"].push_back(params.freq);
	yaml["filter"].push_back(params.Q);
	yaml["filter"].push_back(params.gain);

	return yaml;
}

bool Simple::build_context_widget()
{
	switch (get_context()) {
	case Context::AMPLITUDE_ENVELOPE:
		return params.amplitude_envelope.build_widget("Amplitude");

	case Context::FILTER_ENVELOPE:
		return params.filter_envelope.build_widget("Filter cutoff");

	case Context::FILTER_PARAMETERS:
		return params.svf.build_widget("Filter");

	default:
		return false;
	}
}

static const std::string engine_name{"Simple"};

const std::string &Simple::get_engine_name()
{
	return engine_name;
}

static auto registration = programs.register_engine(engine_name, []()
{
	return std::make_shared<Simple>();
});
