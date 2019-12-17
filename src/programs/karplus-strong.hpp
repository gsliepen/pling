/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <vector>

#include "voice-manager.hpp"
#include "../envelopes/exponential-adsr.hpp"
#include "../filters/state-variable.hpp"
#include "../pling.hpp"
#include "../program.hpp"
#include "../oscillators/basic.hpp"
#include "../pling.hpp"
#include "../program.hpp"

class KarplusStrong: public Program {
	struct Parameters {
		float bend{1};
		float mod{0};
		Envelope::ExponentialADSR::Parameters amplitude_envelope{};
		Envelope::ExponentialADSR::Parameters filter_envelope{};
		float decay{0.9};
	};

	struct Voice {
		Oscillator::Basic osc;
		Oscillator::Basic lfo{10};
		Envelope::ExponentialADSR amplitude_envelope;
		Envelope::ExponentialADSR filter_envelope;

		uint32_t wp;
		std::vector<float> buffer;

		void init(Parameters &params, uint8_t key, float freq, float vel);
		bool render(Chunk &chunk, Parameters &params);
		void release();
		bool is_active() {return amplitude_envelope.is_active();}
		float get_zero_crossing(float offset, Parameters &params);
		float get_frequency(Parameters &params);
	};

	VoiceManager<Voice, 32> voices;

	Parameters params;

	public:
	virtual bool render(Chunk &chunk) final;
	virtual void note_on(uint8_t key, uint8_t vel) final;
	virtual void note_off(uint8_t key, uint8_t vel) final;
	virtual void pitch_bend(int16_t value) final;
	virtual void channel_pressure(int8_t pressure) final;
	virtual void poly_pressure(uint8_t key, uint8_t pressure) final;
	virtual void control_change(uint8_t control, uint8_t val) final;
	virtual void release_all() final;

	virtual float get_zero_crossing(float offset) final;
	virtual float get_base_frequency() final;

	virtual bool build_cc_widget(uint8_t control) final;

	virtual bool load(const YAML::Node &yaml) final;
	virtual YAML::Node save() final;
	virtual const std::string &get_engine_name() final;
};
