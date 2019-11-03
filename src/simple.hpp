/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <vector>

#include "envelope.hpp"
#include "filter.hpp"
#include "oscillator.hpp"
#include "voice-manager.hpp"

class Chunk;

class Simple: public Program {
	struct Parameters {
		float bend{1};
		float mod{0};
		Envelope::ADSR::Parameters adsr{0.1, 0.5, 0.5, 1.0};
		Filter::Biquad::Parameters biquad{};
		Filter::Biquad::Parameters::Type type{};
		Filter::StateVariable::Parameters svf{};
		Filter::StateVariable::Parameters::Type svf_type{};
		float freq{sample_rate / 4};
		float Q{};
		float gain{};
	};

	struct Voice {
		Oscillator osc;
		Oscillator lfo{10};
		float amp;
		uint8_t key;
		bool on;
		Envelope::ADSR adsr;
		Filter::Biquad biquad;
		Filter::StateVariable svf;

		void init(uint8_t key, float freq, float vel);
		void render(Chunk &chunk, Parameters &params);
		void release();
		bool is_active() {return adsr.is_active();}
		float get_zero_crossing(float offset, Parameters &params);
	};

	VoiceManager<Voice, 32> voices;

	Parameters params;

	public:
	virtual void render(Chunk &chunk) final;
	virtual void note_on(uint8_t key, uint8_t vel) final;
	virtual void note_off(uint8_t key, uint8_t vel) final;
	virtual void pitch_bend(int16_t value) final;
	virtual void channel_pressure(int8_t pressure) final;
	virtual void poly_pressure(uint8_t key, uint8_t pressure) final;
	virtual void control_change(uint8_t control, uint8_t val) final;

	virtual float get_zero_crossing(float offset) final;
};
