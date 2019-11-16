/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pling.hpp"

#include <fftw3.h>
#include <fmt/ostream.h>
#include <glm/glm.hpp>
#include <iostream>
#include <SDL2/SDL.h>
#include <set>

#include "midi.hpp"
#include "program-manager.hpp"
#include "text.hpp"
#include "ui.hpp"
#include "widgets/oscilloscope.hpp"
#include "widgets/spectrum.hpp"

static RingBuffer signal{16384};
static ProgramManager programs;

static void audio_callback(void *userdata, uint8_t *stream, int len) {
	static Chunk chunk;

	/* Render samples from active programs */
	programs.render(chunk);

	/* Add the samples to the oscilloscope */
	signal.add(chunk, programs.get_zero_crossing(-384));

	/* Convert to 16-bit signed stereo */
	int nsamples = len / 4;
	int16_t *data = (int16_t *)stream;

	for(int i = 0; i < nsamples; i++) {
		*data++ = glm::clamp(chunk.samples[i] * 0.1f, -1.f, 1.f) * 32767;
		*data++ = glm::clamp(chunk.samples[i] * 0.1f, -1.f, 1.f) * 32767;
	}
}

static void setup_audio() {
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

int main(int argc, char *args[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	setup_audio();

	fftwf_import_wisdom_from_filename("pling.wisdom");
	UI ui(signal);

	MIDI::Manager midi_manager(programs);

	ui.run();

	fftwf_export_wisdom_to_filename("pling.wisdom");
	SDL_Quit();
}
