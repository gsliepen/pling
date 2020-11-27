/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "karplus-strong.hpp"

#include <cmath>
#include <fmt/ostream.h>
#include <iostream>
#include <random>

#include "pling.hpp"
#include "../program-manager.hpp"
#include "utils.hpp"

static std::random_device random_device;
static std::default_random_engine random_engine(random_device());
static std::uniform_real_distribution<float> uniform_distribution(-1.0f, 1.0f);

bool KarplusStrong::Voice::render(Chunk &chunk, Parameters &params) {
	for (auto &sample: chunk.samples) {
		float decay_envelope = filter_envelope.update(params.filter_envelope);

		float rp = osc() * buffer.size();
		uint32_t rp1 = rp;
		uint32_t rp2 = (rp1 + 1);
		if (rp2 >= buffer.size())
			rp2 = 0;
		float a = rp - rp1;

		/* Linear interpolation for the output. */
		sample += ((1.0f - a) * buffer[rp1] + a * buffer[rp2]) * amplitude_envelope.update(params.amplitude_envelope) * (1 - (lfo.fast_sine() * 0.5 + 0.5) * params.mod);

		++lfo;
		osc.update(params.bend);

		if (uint32_t(osc() * buffer.size()) == rp1)
			continue;

		/* We reached the next sample, update the buffer */
		uint32_t rp3 = rp1 ? (rp1 - 1) : buffer.size() - 1;
		buffer[rp1] = buffer[rp1] * (params.decay * decay_envelope) + (buffer[rp2] + buffer[rp3]) * 0.5f * (1.0f - (params.decay * decay_envelope));
	}

	return is_active();
}

void KarplusStrong::Voice::init(Parameters &params, uint8_t key, float freq, float amp) {
	buffer.resize(lrintf(sample_rate / freq), 0.0f);

	lfo.init(10);
	osc.init(freq);
	amplitude_envelope.init();
	filter_envelope.init();

	/* Use random excitation for the delay line */
	for (auto &sample: buffer)
		sample = uniform_distribution(random_engine) * amp * 2.0f;
}

void KarplusStrong::Voice::release() {
	amplitude_envelope.release();
	filter_envelope.release();
}

float KarplusStrong::Voice::get_zero_crossing(float offset, KarplusStrong::Parameters &params) {
	return osc.get_zero_crossing(offset, params.bend);
}

float KarplusStrong::Voice::get_frequency(KarplusStrong::Parameters &params) {
	return osc.get_frequency(params.bend);
}

bool KarplusStrong::render(Chunk &chunk) {
	bool active = false;

	for(auto &voice: voices) {
		active |= voice.render(chunk, params);
	}

	return active;
}

float KarplusStrong::get_zero_crossing(float offset) {
	float crossing = offset;

	if(Voice *lowest = voices.get_lowest(); lowest) {
		crossing = lowest->get_zero_crossing(offset, params);
	}

	return crossing;
}

float KarplusStrong::get_base_frequency() {
	if(Voice *lowest = voices.get_lowest(); lowest)
		return lowest->get_frequency(params);
	else
		return {};
}

void KarplusStrong::note_on(uint8_t key, uint8_t vel) {
	Voice *voice = voices.press(key);
	if (!voice)
		return;

	float freq = key_to_frequency(key);
	float amp = cc_exponential(vel, 1.0f / 32.0f, 1.0f);
	voice->init(params, key, freq, amp);
}

void KarplusStrong::note_off(uint8_t key, uint8_t vel) {
	voices.release(key);
}

void KarplusStrong::pitch_bend(int16_t value) {
	params.bend = std::exp2(value / 8192.0 / 6.0);
}

void KarplusStrong::channel_pressure(int8_t pressure) {}
void KarplusStrong::poly_pressure(uint8_t key, uint8_t pressure) {}
void KarplusStrong::set_fader(MIDI::Control control, uint8_t val) {
	switch(control.col) {
	case 0:
		params.amplitude_envelope.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 1:
		params.amplitude_envelope.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 2:
		params.amplitude_envelope.set_sustain(dB_to_amplitude(cc_linear(val, -48, 0)));
		break;
	case 3:
		params.amplitude_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 4:
		params.filter_envelope.set_attack(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 5:
		params.filter_envelope.set_decay(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 6:
		params.filter_envelope.set_sustain(dB_to_amplitude(cc_linear(val, -48, 0)));
		break;
	case 7:
		params.filter_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
		break;

	default:
		break;
	}
}

void KarplusStrong::set_pot(MIDI::Control control, uint8_t val) {
	switch(control.col) {
	case 0:
		params.decay = 1.0f - cc_exponential(127 - val, 0.001, 1);
		break;
	default:
		break;
	}
}

void KarplusStrong::set_button(MIDI::Control control, uint8_t val) {
}

void KarplusStrong::modulation(uint8_t val) {
	params.mod = cc_linear(val, 0, 1);
}

void KarplusStrong::sustain(bool val) {
	voices.set_sustain(val);
}

void KarplusStrong::release_all() {
	voices.release_all();
}

bool KarplusStrong::load(const YAML::Node &yaml) {
	params.amplitude_envelope.set_attack(yaml["amplitude_envelope"][0].as<float>());
	params.amplitude_envelope.set_decay(yaml["amplitude_envelope"][1].as<float>());
	params.amplitude_envelope.set_sustain(yaml["amplitude_envelope"][2].as<float>());
	params.amplitude_envelope.set_release(yaml["amplitude_envelope"][3].as<float>());

	params.filter_envelope.set_attack(yaml["filter_envelope"][0].as<float>());
	params.filter_envelope.set_decay(yaml["filter_envelope"][1].as<float>());
	params.filter_envelope.set_sustain(yaml["filter_envelope"][2].as<float>());
	params.filter_envelope.set_release(yaml["filter_envelope"][3].as<float>());

	params.decay = yaml["decay"].as<float>();

	return true;
}

YAML::Node KarplusStrong::save() {
	YAML::Node yaml;

	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.attack);
	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.decay);
	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.sustain);
	yaml["amplitude_envelope"].push_back(params.amplitude_envelope.release);

	yaml["filter_envelope"].push_back(params.filter_envelope.attack);
	yaml["filter_envelope"].push_back(params.filter_envelope.decay);
	yaml["filter_envelope"].push_back(params.filter_envelope.sustain);
	yaml["filter_envelope"].push_back(params.filter_envelope.release);

	yaml["decay"] = params.decay;

	return yaml;
}

bool KarplusStrong::build_cc_widget(uint8_t control) {
	switch(control) {
	case 12:
	case 38:
	case 13:
	case 39:
	case 14:
	case 40:
	case 15:
	case 41:
		return params.amplitude_envelope.build_widget("Amplitude");
	case 16:
	case 42:
	case 17:
	case 43:
	case 18:
	case 44:
	case 19:
	case 45:
		return params.filter_envelope.build_widget("Filter cutoff");

	default:
		return false;
	}
}

static const std::string engine_name{"Karplus-Strong"};

const std::string &KarplusStrong::get_engine_name() {
	return engine_name;
}

static auto registration = programs.register_engine(engine_name, [](){return std::make_shared<KarplusStrong>();});
