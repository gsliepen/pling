/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <vector>

#include "voice-manager.hpp"
#include "../controller.hpp"
#include "../envelopes/exponential-dx7.hpp"
#include "../filters/state-variable.hpp"
#include "../pling.hpp"
#include "../program.hpp"
#include "../oscillators/pm.hpp"
#include "../pling.hpp"
#include "../program.hpp"

class Octalope: public Program
{
	struct Operator {
		struct Parameters {
			static constexpr uint8_t NO_SOURCE = 255;
			uint8_t mod_source{NO_SOURCE};
			uint8_t keyboard_breakpoint;
			float freq_ratio{1};
			float detune{};
			float output_level{};
			float fm_level{};
			float am_level{};
			float rm_level{};
			float keyboard_scaling[2];
			Envelope::ExponentialDX7::Parameters envelope{};
			uint8_t waveform;
			uint8_t freq_coarse;
			uint8_t freq_fine;
		};

		Oscillator::PM osc;
		Envelope::ExponentialDX7 envelope;
		float value{};
		float prev{};
	};

	struct Parameters {
		float bend{1};
		float freq{sample_rate / 4};
		float Q{};
		Envelope::ExponentialDX7::Parameters freq_envelope{};
		Envelope::ExponentialDX7::Parameters filter_envelope{};
		Operator::Parameters ops[8] = {
			{1, 60, 1, 0, 1},
			{2, 60, 2, 0, 0},
			{3, 60, 3, 0, 0},
			{4, 60, 4, 0, 0},
			{5, 60, 5, 0, 0},
			{6, 60, 6, 0, 0},
			{7, 60, 7, 0, 0},
			{7, 60, 8, 0, 0},
		};
		Filter::StateVariable::Parameters svf{};
		Filter::StateVariable::Parameters::Type svf_type{};
	};

	struct Voice {
		float amp;
		float freq;
		float delta;
		Envelope::ExponentialDX7 freq_envelope;
		Envelope::ExponentialDX7 filter_envelope;
		Operator ops[8];
		Filter::StateVariable svf;

		void init(uint8_t key, float freq, float vel, const Parameters &params);
		bool render(Chunk &chunk, Parameters &params);
		void release();
		bool is_active()
		{
			return ops[0].envelope.is_active();
		}
		float get_zero_crossing(float offset, const Parameters &params) const;
		float get_frequency(const Parameters &params) const;
	};

	VoiceManager<Voice, 32> voices;

	Parameters params;

	enum class Context {
		NONE,
		MAIN,
		ENVELOPE,
	} current_context{};

	uint8_t current_op{};
	bool current_op_held{};

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

	bool build_main_widget();

	void set_envelope(MIDI::Control control, uint8_t val, Envelope::ExponentialDX7::Parameters &envelope);

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
