/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "clock.hpp"
#include "state.hpp"

void State::set_pot(MIDI::Control control, MIDI::Port &port, int8_t value)
{
	if (mode == Mode::INSTRUMENT) {
		auto program = programs.get_last_activated_program();
		program->set_pot(control, value);
	}
}

void State::set_fader(MIDI::Control control, MIDI::Port &port, int8_t value)
{
	if (control.master) {
		state.set_master_volume(value ? dB_to_amplitude(value / 127.0f * 48.0f - 48.0f) : 0);
		return;
	}

	if (mode == Mode::INSTRUMENT) {
		auto program = programs.get_last_activated_program();
		program->set_fader(control, value);
	}
}

void State::set_button(MIDI::Control control, MIDI::Port &port, int8_t value)
{
	if (mode == Mode::INSTRUMENT) {
		auto program = programs.get_last_activated_program();
		program->set_button(control, value);
	}
}

void State::process_control(MIDI::Control control, MIDI::Port &port, uint8_t value)
{
	switch (control.command) {
	/* Generic controls */
	case MIDI::Command::PASS:
	case MIDI::Command::IGNORE:
		return;

	case MIDI::Command::POT:
		set_pot(control, port, value);
		break;

	case MIDI::Command::FADER:
		set_fader(control, port, value);
		break;

	case MIDI::Command::BUTTON:
		set_button(control, port, value);
		break;

	case MIDI::Command::PAD:
	case MIDI::Command::GRID:

	/* Transport control */
	case MIDI::Command::LOOP:
		break;

	case MIDI::Command::REWIND:
		master_clock.reset();
		break;

	case MIDI::Command::FORWARD:
		break;

	case MIDI::Command::STOP:
		master_clock.stop();
		break;

	case MIDI::Command::PLAY:
		master_clock.start();
		break;

	case MIDI::Command::RECORD:

	/* Other actions */
	case MIDI::Command::HOME:
	case MIDI::Command::SET_LEFT:
	case MIDI::Command::SET_RIGHT:
	case MIDI::Command::UNDO:
	case MIDI::Command::CLICK:
	case MIDI::Command::MODE:
	case MIDI::Command::MIXER:
	case MIDI::Command::INSTRUMENT:
	case MIDI::Command::PRESET:
	case MIDI::Command::BANK:
	case MIDI::Command::CLIPS:
	case MIDI::Command::SCENES:
	case MIDI::Command::PAGES:
	case MIDI::Command::SHIFT:
	case MIDI::Command::TRACK:
	case MIDI::Command::PATTERN:
	case MIDI::Command::TEMPO:
	case MIDI::Command::CROSSFADE:
		break;
	}
}

void State::build_context_widget(void)
{
	if (auto program = programs.get_last_activated_program()) {
		program->build_context_widget();
	}
}
