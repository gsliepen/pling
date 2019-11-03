/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pling.hpp"

#include <fmt/ostream.h>
#include <glm/glm.hpp>
#include <iostream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>
#include <set>
#include <thread>

#include "envelope.hpp"
#include "midi.hpp"
#include "oscilloscope.hpp"
#include "simple.hpp"
#include "text.hpp"

static Font::Manager font_manager;
static Text text;
static Oscilloscope::Signal osc_signal;

Simple program;

static bool handle_events() {
	for (SDL_Event ev; SDL_PollEvent(&ev);) {
		switch (ev.type) {
		case SDL_QUIT:
			return false;
		}
	}

	return true;
}

static void render(Oscilloscope::Widget &oscilloscope) {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	Text::prepare_render(800, 480);
	text.render();
	oscilloscope.render(800, 480);
}

static void ui() {
	uint32_t window_flags = SDL_WINDOW_OPENGL;

	SDL_DisplayMode desktop_mode;
	SDL_GetDesktopDisplayMode(0, &desktop_mode);

	if(desktop_mode.w == 800 && desktop_mode.h == 480)
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	auto window = SDL_CreateWindow("Pling", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 480, window_flags);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1);
	auto context = SDL_GL_CreateContext(window);

	Text::init();
	text.set_font(font_manager, "/usr/share/fonts/truetype/freefont/FreeSans.ttf", 12);
	text.set_text("Pling!");
	text.set_position(16, 16);

	Oscilloscope::Widget oscilloscope(osc_signal);
	oscilloscope.set_position(16, 200, 768, 256);

	while (handle_events()) {
		render(oscilloscope);
		SDL_GL_SwapWindow(window);
	}

	SDL_GL_DeleteContext(context);
	SDL_DestroyWindow(window);
}

static void audio_callback(void *userdata, uint8_t *stream, int len) {
	static Chunk chunk;

	/* Render samples from current program */
	program.render(chunk);

	/* Add the samples to the oscilloscope */
	osc_signal.add(chunk, program.get_zero_crossing(-384));

	/* Convert to 16-bit signed stereo */
	int nsamples = len / 4;
	int16_t *data = (int16_t *)stream;

	for(int i = 0; i < nsamples; i++) {
		*data++ = glm::clamp(chunk.samples[i] * 0.1f, -1.f, 1.f) * 32767;
		*data++ = glm::clamp(chunk.samples[i] * 0.1f, -1.f, 1.f) * 32767;
	}
}

static void audio() {
	SDL_AudioSpec want{}, have{};

	want.freq = sample_rate;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = chunk_size;
	want.callback = audio_callback;

	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);

	if (!dev)
		throw std::runtime_error(SDL_GetError());

	SDL_PauseAudioDevice(dev, 0);
}

static void note_off(MIDI::Port &port, uint8_t chan, uint8_t key, uint8_t vel) {
	program.note_off(key, vel);
}

static void note_on(MIDI::Port &port, uint8_t chan, uint8_t key, uint8_t vel) {
	program.note_on(key, vel);
}

static void poly_pressure(MIDI::Port &port, uint8_t chan, uint8_t key, uint8_t pressure) {
	program.poly_pressure(key, pressure);
}

static void control_change(MIDI::Port &port, uint8_t chan, uint8_t control, uint8_t value) {
	program.control_change(control, value);
}

static void program_change(MIDI::Port &port, uint8_t chan, uint8_t program) {

}

static void channel_pressure(MIDI::Port &port, uint8_t chan, uint8_t pressure) {
	program.channel_pressure(pressure);
}

static void pitch_bend(MIDI::Port &port, uint8_t chan, uint8_t lsb, uint8_t msb) {
	int16_t value = msb << 7 | lsb;
	program.pitch_bend(value - 8192);
}

static void midi_callback(MIDI::Port &port, uint8_t *data, ssize_t len) {
	fmt::print(std::cerr, "Got {} bytes from MIDI port {}\n", len, (void *)&port);

	while (len) {
		uint8_t type = data[0] >> 4;
		uint8_t chan = data[0] & 0xf;
		ssize_t parsed = 0;

		switch(type) {
		case 0x0:
		case 0x1:
		case 0x2:
		case 0x3:
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
			// This should never happen in the first byte.
			break;

		// Channel voice messages
		// ----------------------
		case 0x8:
			note_off(port, chan, data[1], data[2]);
			parsed = 3;
			break;
		case 0x9:
			if (data[2]) {
				note_on(port, chan, data[1], data[2]);
			} else {
				note_off(port, chan, data[1], data[2]);
			}
			parsed = 3;
			break;
		case 0xa:
			poly_pressure(port, chan, data[1], data[2]);
			parsed = 3;
			break;
		case 0xb:
			control_change(port, chan, data[1], data[2]);
			parsed = 3;
			break;
		case 0xc:
			program_change(port, chan, data[1]);
			parsed = 2;
			break;
		case 0xd:
			channel_pressure(port, chan, data[1]);
			parsed = 2;
			break;
		case 0xe:
			pitch_bend(port, chan, data[1], data[2]);
			parsed = 3;
			break;
		case 0xf:
			switch (data[0] & 0x0f) {
			// System common messages
			// ----------------------
			case 0x0: // system exclusive
				break;
			case 0x1: // time code
				break;
			case 0x2: // song position pointer
				break;
			case 0x3: // song select
				break;
			case 0x4: // (reserved)
				break;
			case 0x5: // (reserved)
				break;
			case 0x6: // tune request
				break;
			case 0x7: // end of system exclusive
				break;

			// System real-time messages
			// -------------------------
			case 0x8: // timing clock
				break;
			case 0x9: // (reserved)
				break;
			case 0xa: // start
				break;
			case 0xb: // continue
				break;
			case 0xc: // stop
				break;
			case 0xd: // (undefined)
				break;
			case 0xe: // active sensing
				break;
			case 0xf: // reset
				break;
			}
			parsed = 1;
			while(parsed < len && data[parsed] < 0x80)
				parsed++;
			break;
		}

		len -= parsed;
		data += parsed;
	}
}

static void midi() {
	MIDI::Manager midi_manager;

	while(true)
		midi_manager.process_events(midi_callback);
}

int main(int argc, char *args[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	audio();
	std::thread midi_thread(midi);
	ui();

	SDL_Quit();
}
