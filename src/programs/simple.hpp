/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <vector>

#include "voice-manager.hpp"
#include "../controller.hpp"
#include "../envelopes/exponential-adsr.hpp"
#include "../filters/state-variable.hpp"
#include "../pling.hpp"
#include "../program.hpp"
#include "../oscillators/basic.hpp"
#include "../pling.hpp"
#include "../program.hpp"

class Simple: public Program
{
	struct Parameters {
		float bend{1};
		float mod{0};
		Envelope::ExponentialADSR::Parameters amplitude_envelope{};
		Envelope::ExponentialADSR::Parameters filter_envelope{};
		Filter::StateVariable::Parameters svf{};
		Filter::StateVariable::Parameters::Type svf_type{};
		float freq{sample_rate / 4};
		float Q{};
		float gain{};
	};

	struct Voice {
		Oscillator::Basic osc;
		Oscillator::Basic lfo{10};
		float amp;
		Envelope::ExponentialADSR amplitude_envelope;
		Envelope::ExponentialADSR filter_envelope;
		Filter::StateVariable svf;

		void init(uint8_t key, float freq, float vel);
		bool render(Chunk &chunk, Parameters &params);
		void release();
		bool is_active()
		{
			return amplitude_envelope.is_active();
		}
		float get_zero_crossing(float offset, const Parameters &params) const;
		float get_frequency(const Parameters &params) const;
	};

	VoiceManager<Voice, 32> voices;

	Parameters params;

	enum class Context {
		NONE,
		AMPLITUDE_ENVELOPE,
		FILTER_ENVELOPE,
		FILTER_PARAMETERS,
	} current_context{};

	using clock = std::chrono::steady_clock;
	clock::time_point last_context_change{};

	void set_context(Context context)
	{
		current_context = context;
		last_context_change = clock::now();
	}

	Context get_context()
	{
		if (clock::now() - last_context_change > std::chrono::seconds(10)) {
			current_context = {};
		}

		return current_context;
	}

public:
	virtual bool render(Chunk &chunk) final;
	virtual void note_on(uint8_t key, uint8_t vel) final;
	virtual void note_off(uint8_t key, uint8_t vel) final;
	virtual void pitch_bend(int16_t value) final;
	virtual void channel_pressure(int8_t pressure) final;
	virtual void poly_pressure(uint8_t key, uint8_t pressure) final;
	virtual void modulation(uint8_t value) final;
	virtual void sustain(bool value) final;
	virtual void release_all() final;

	virtual void set_fader(MIDI::Control control, uint8_t val) final;
	virtual void set_pot(MIDI::Control control, uint8_t val) final;
	virtual void set_button(MIDI::Control control, uint8_t val) final;

	virtual float get_zero_crossing(float offset) const final;
	virtual float get_base_frequency() const final;

	virtual bool build_context_widget(void) final;

	virtual bool load(const YAML::Node &yaml) final;
	virtual YAML::Node save() final;
	virtual const std::string &get_engine_name() final;
};
