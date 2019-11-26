/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "simple.hpp"

#include <cmath>
#include <fmt/ostream.h>
#include <iostream>

#include "pling.hpp"
#include "utils.hpp"

bool Simple::Voice::render(Chunk &chunk, Parameters &params) {
	for (auto &sample: chunk.samples) {
		params.svf.set_freq(filter_envelope.update(params.filter_envelope) * params.freq);
		sample += svf(params.svf, osc.saw() * amp * amplitude_envelope.update(params.amplitude_envelope) * (1 - (lfo.fast_sine() * 0.5 + 0.5) * params.mod));
		++lfo;
		osc.update(params.bend);
	}

	return is_active();
}

void Simple::Voice::init(uint8_t key, float freq, float amp) {
	lfo.init(10);
	osc.init(freq);
	this->amp = amp;
	this->key = key;
	amplitude_envelope.init();
	filter_envelope.init();
}

void Simple::Voice::release() {
	amplitude_envelope.release();
	filter_envelope.release();
}

float Simple::Voice::get_zero_crossing(float offset, Simple::Parameters &params) {
	return osc.get_zero_crossing(offset, params.bend);
}

bool Simple::render(Chunk &chunk) {
	bool active = false;

	for(auto &voice: voices) {
		active |= voice.render(chunk, params);
	}

	return active;
}

float Simple::get_zero_crossing(float offset) {
	float crossing = offset;

	if(Voice *lowest = voices.get_lowest(); lowest) {
		crossing = lowest->get_zero_crossing(offset, params);
	}

	return crossing;
}

void Simple::note_on(uint8_t key, uint8_t vel) {
	Voice *voice = voices.press(key);
	if (!voice)
		return;

	float freq = 440.0 * powf(2.0, (key - 69) / 12.0);
	float amp = expf((vel - 127.) / 32.);
	voice->init(key, freq, amp);
}

void Simple::note_off(uint8_t key, uint8_t vel) {
	voices.release(key);
}

void Simple::pitch_bend(int16_t value) {
	params.bend = powf(2.0, value / 8192.0 / 6.0);
}

void Simple::channel_pressure(int8_t pressure) {}
void Simple::poly_pressure(uint8_t key, uint8_t pressure) {}
void Simple::control_change(uint8_t control, uint8_t val) {
	switch(control) {
	case 1:
		params.mod = cc_linear(val, 0, 1);
		break;
	case 12:
	case 38:
		params.amplitude_envelope.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 13:
	case 39:
		params.amplitude_envelope.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 14:
	case 40:
		params.amplitude_envelope.set_sustain(cc_exponential(val, 0, 1e-2, 1, 1));
		break;
	case 15:
	case 41:
		params.amplitude_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 16:
	case 42:
		params.filter_envelope.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 17:
	case 43:
		params.filter_envelope.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 18:
	case 44:
		params.filter_envelope.set_sustain(cc_exponential(val, 0, 1e-2, 1, 1));
		break;
	case 19:
	case 45:
		params.filter_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 30:
	case 60:
		params.freq = cc_exponential(val, 0, 1, 1e4, sample_rate / 4);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;
	case 31:
	case 61:
		params.Q = cc_exponential(val, 0, 1e-2, 1e2, 1e2);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;
	case 32:
	case 62:
		params.gain = cc_exponential(val, 0, 1e-2, 1e2, 1e2);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;

	case 33:
	case 63:
		params.svf_type = static_cast<Filter::StateVariable::Parameters::Type>(cc_select(val, 4));
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {} {}\n", params.freq, params.Q, params.gain, (int)params.svf_type);
		break;

	case 64:
		voices.set_sustain(val);
		break;

	default:
		fmt::print(std::cerr, "{} {}\n", control, val);
		break;
	}
}

void Simple::release_all() {
	voices.release_all();
}
