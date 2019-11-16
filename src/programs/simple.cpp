/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "simple.hpp"

#include <cmath>
#include <fmt/ostream.h>
#include <iostream>

#include "pling.hpp"
#include "utils.hpp"

bool Simple::Voice::render(Chunk &chunk, Parameters &params) {
	for (auto &sample: chunk.samples) {
		sample += svf(params.svf, osc.square() * amp * adsr.update(params.adsr) * (1 - (lfo.fast_sine() * 0.5 + 0.5) * params.mod));
		++lfo;
		osc.update(params.bend);
	}

	return adsr.is_active();
}

void Simple::Voice::init(uint8_t key, float freq, float amp) {
	lfo.init(10);
	osc.init(freq);
	this->amp = amp;
	this->key = key;
	adsr.init();
}

void Simple::Voice::release() {
	adsr.release();
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
	case 3:
	case 38:
		params.adsr.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 4:
	case 39:
		params.adsr.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 5:
	case 40:
		params.adsr.set_sustain(cc_exponential(val, 0, 1e-2, 1, 1));
		break;
	case 6:
	case 41:
		params.adsr.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 14:
	case 60:
		params.freq = cc_exponential(val, 0, 1, 1e4, sample_rate / 4);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;
	case 15:
	case 61:
		params.Q = cc_exponential(val, 0, 1e-2, 1e2, 1e2);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;
	case 16:
	case 62:
		params.gain = cc_exponential(val, 0, 1e-2, 1e2, 1e2);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;

	case 17:
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
