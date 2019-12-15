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
	params.bend = powf(2.0, value / 8192.0 / 6.0);
}

void KarplusStrong::channel_pressure(int8_t pressure) {}
void KarplusStrong::poly_pressure(uint8_t key, uint8_t pressure) {}
void KarplusStrong::control_change(uint8_t control, uint8_t val) {
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
		params.filter_envelope.set_sustain(1.0f - cc_exponential(127 - val, 0.001, 1));
		break;
	case 19:
	case 45:
		params.filter_envelope.set_release(cc_exponential(val, 0, 1e-2, 1e1, 1e1));
		break;
	case 30:
	case 60:
		params.decay = 1.0f - cc_exponential(127 - val, 0.001, 1);
		break;
	case 31:
	case 61:
		break;
	case 32:
	case 62:
		break;

	case 33:
	case 63:
		break;

	case 64:
		voices.set_sustain(val);
		break;

	default:
		fmt::print(std::cerr, "{} {}\n", control, val);
		break;
	}
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

static const std::string engine_name{"Karplus-Strong"};

const std::string &KarplusStrong::get_engine_name() {
	return engine_name;
}

static auto registration = programs.register_engine(engine_name, [](){return std::make_shared<KarplusStrong>();});
