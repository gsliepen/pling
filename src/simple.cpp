/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "simple.hpp"

#include <cmath>
#include <fmt/ostream.h>
#include <iostream>

#include "pling.hpp"

void Simple::Voice::render(Chunk &chunk, Parameters &params) {
	for (auto &sample: chunk.samples) {
		sample += svf(params.svf, osc.square() * amp * adsr.update(params.adsr, on) * (1 - (lfo * 0.5 + 0.5) * params.mod));
		++lfo;
		osc.update(params.bend);
	}
}

void Simple::Voice::init(uint8_t key, float freq, float amp) {
	lfo.init(10);
	osc.init(freq);
	this->amp = amp;
	this->key = key;
	on = true;
	adsr.init();
}

void Simple::Voice::release() {
	on = false;
}

float Simple::Voice::get_zero_crossing(float offset, Simple::Parameters &params) {
	return osc.get_zero_crossing(offset, params.bend);
}

void Simple::render(Chunk &chunk) {
	chunk.clear();

	for(auto &voice: voices) {
		voice.render(chunk, params);
	}
}

float Simple::get_zero_crossing(float offset) {
	float crossing = offset;
	uint8_t key = 255;

	for (auto &voice: voices) {
		if (voice.key < key) {
			key = voice.key;
			crossing = voice.get_zero_crossing(offset, params);
		}
	}

	return crossing;
}

void Simple::note_on(uint8_t key, uint8_t vel) {
	float freq = 440.0 * powf(2.0, (key - 69) / 12.0);
	float amp = expf((vel - 127.) / 32.);
	voices.get(key).init(key, freq, amp);
}

void Simple::note_off(uint8_t key, uint8_t vel) {
	voices.get(key).release();
}

void Simple::pitch_bend(int16_t value) {
	params.bend = powf(2.0, value / 8192.0 / 6.0);
}

void Simple::channel_pressure(int8_t pressure) {}
void Simple::poly_pressure(uint8_t key, uint8_t pressure) {}
void Simple::control_change(uint8_t control, uint8_t val) {
	switch(control) {
	case 1:
		params.mod = val / 127.0;
		break;
	case 3:
	case 38:
		params.adsr.attack = val ? powf(2.0, (val - 64.0) / 16.0) : 0;
		break;
	case 4:
	case 39:
		params.adsr.decay = val ? powf(2.0, (val - 64.0) / 16.0) : 0;
		break;
	case 5:
	case 40:
		params.adsr.sustain = val ? powf(2.0, (val - 127.0) / 16.0) : 0;
		break;
	case 6:
	case 41:
		params.adsr.release = val ? powf(2.0, (val - 64.0) / 16.0) : 0;
		break;
	case 14:
	case 60:
		params.freq = sample_rate / 4 * powf(2.0, (val - 128.0) / 16.0);
		params.biquad.set(params.type, params.freq, params.Q, params.gain);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;
	case 15:
	case 61:
		params.Q = val ? powf(2.0, (val - 64.0) / 16.0) : 0;
		params.biquad.set(params.type, params.freq, params.Q, params.gain);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;
	case 16:
	case 62:
		params.gain = (val - 64.0) / 4.0;
		params.biquad.set(params.type, params.freq, params.Q, params.gain);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {}\n", params.freq, params.Q, params.gain);
		break;

	case 17:
	case 63:
		params.type = static_cast<Filter::Biquad::Parameters::Type>(val % 7);
		params.svf_type = static_cast<Filter::StateVariable::Parameters::Type>(val % 4);
		params.biquad.set(params.type, params.freq, params.Q, params.gain);
		params.svf.set(params.svf_type, params.freq, params.Q);
		fmt::print(std::cerr, "{} {} {} {} {}\n", params.freq, params.Q, params.gain, val % 7, val % 4);
		break;
	default:
		fmt::print(std::cerr, "{} {}\n", control, val);
		break;
	}
}
