/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <vector>

#include "voice-manager.hpp"
#include "../controller.hpp"
#include "../curves/keyboard-scaling-dx7.hpp"
#include "../curves/velocity-scaling-dx7.hpp"
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
			float frequency{1};
			float detune{};
			float output_level{};
			uint8_t waveform{};
			uint8_t frequency_coarse{64};
			uint8_t frequency_fine{};
			bool fixed{};
			bool sync{};
			bool tempo{};
			float fm_level[8] {};
			float am_level;
			float mod_sensitivity{};

			Envelope::ExponentialDX7::Parameters envelope{};
			Curve::KeyboardScalingDX7 keyboard_level_curve{440, false, false, 0, 0};
			Curve::KeyboardScalingDX7 keyboard_rate_curve{440, false, false, 0, 0};
			Curve::VelocityScalingDX7 velocity_level_curve{1.0, true, false, -6, 0};
			Curve::VelocityScalingDX7 velocity_rate_curve{1.0, false, false, 0, 0};

			void set_frequency_coarse(uint8_t val)
			{
				frequency_coarse = val;
				update_frequency();
			};
			void set_frequency_fine(uint8_t val)
			{
				frequency_fine = val;
				update_frequency();
			};
			void set_detune(uint8_t val);
			void set_flags(uint8_t val);

		private:
			void update_frequency();
		};

		Oscillator::PM osc;
		Envelope::ExponentialDX7 envelope;
		float output_level;
		float rate{1};
		float value{};
	};

	struct Parameters {
		float bend{};
		float modulation{};

		struct Frequency {
			float transpose{1};
			float randomize{};
			float lfo_depth{};
			float bend_sensitivity{2};
			float mod_sensitivity{};
			bool tempo{};
			Envelope::ExponentialDX7::Parameters envelope{};
		} frequency;

		Operator::Parameters ops[8] {};

		struct Filter {
			float lfo_depth{};
			::Filter::StateVariable::Parameters svf{};
			Envelope::ExponentialDX7::Parameters envelope{};

			float frequency{1};
			float Q{1};
			float randomize{};
			uint8_t frequency_coarse{64};
			uint8_t frequency_fine{};
			bool fixed{};
			bool tempo{};
			float bend_sensitivity{0};
			float mod_sensitivity{};
			::Filter::StateVariable::Parameters::Type type{};

			void set_frequency_coarse(uint8_t val)
			{
				frequency_coarse = val;
				update_frequency();
			}

			void set_frequency_fine(uint8_t val)
			{
				frequency_fine = val;
				update_frequency();
			}

			void set_flags(uint8_t val);
		private:
			void update_frequency();
		} filter;
	};

	struct Voice {

		struct {
			Envelope::ExponentialDX7 envelope;
			float base;
			float rate{1};
		} frequency;

		struct {
			Envelope::ExponentialDX7 envelope;
			Filter::StateVariable svf;
			float base;
			float rate{1};
		} filter;

		Operator ops[8];

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

	enum class Page {
		OPERATOR_WAVEFORM,
		OPERATOR_MODULATION,
		OPERATOR_SCALING,
		GLOBAL_PITCH,
		GLOBAL_FILTER,
	} current_page{};

	int current_op{};

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

	bool build_operator_waveform_widget();
	bool build_operator_modulation_widget();
	bool build_operator_scaling_widget();
	bool build_global_pitch_widget();
	bool build_global_filter_widget();

	void set_envelope(MIDI::Control control, uint8_t val, Envelope::ExponentialDX7::Parameters &envelope, float from, float to);

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
