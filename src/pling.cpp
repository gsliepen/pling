/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pling.hpp"

#include <fftw3.h>
#include <filesystem>
#include <fmt/ostream.h>
#include <glm/glm.hpp>
#include <iostream>
#include <SDL2/SDL.h>
#include <set>

#include "midi.hpp"
#include "program-manager.hpp"
#include "ui.hpp"
#include "state.hpp"
#include "widgets/oscilloscope.hpp"
#include "widgets/spectrum.hpp"

static RingBuffer ringbuffer{16384};
Program::Manager programs;
YAML::Node global_config;

State state;
MIDI::Manager MIDI::manager(programs);

static void audio_callback(void *userdata, uint8_t *stream, int len) {
	static Chunk chunk;

	/* Render samples from active programs */
	programs.render(chunk);

	/* Add the samples to the oscilloscope */
	ringbuffer.add(chunk, programs.get_zero_crossing(-384), programs.get_base_frequency());

	/* Convert to 16-bit signed stereo */
	int nsamples = len / 4;
	int16_t *data = (int16_t *)stream;
	float amplitude = state.get_master_volume() * 0.25f; // leave ~12 dB headroom

	for(int i = 0; i < nsamples; i++) {
		*data++ = glm::clamp(chunk.samples[i] * amplitude, -1.f, 1.f) * 32767;
		*data++ = glm::clamp(chunk.samples[i] * amplitude, -1.f, 1.f) * 32767;
	}
}

static void setup_audio() {
	SDL_AudioSpec want{}, have{};

	want.freq = sample_rate;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = chunk_size;
	want.callback = audio_callback;

	auto name = global_config["audio_device"].as<std::string>("");

	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(name.c_str(), 0, &want, &have, 0);

	if (!dev) {
		fmt::print(std::cerr, "Could not open {}\n", name);
		dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	}

	if (!dev)
		throw std::runtime_error(SDL_GetError());

	SDL_PauseAudioDevice(dev, 0);
}

static void read_config(const std::filesystem::path &filename) {
	global_config = YAML::LoadFile(filename);
}

int main(int argc, char *args[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

	char *pref_path = SDL_GetPrefPath(NULL, "pling");
	read_config(std::filesystem::path(pref_path) / "config.yaml");

	setup_audio();
	MIDI::manager.start();

	fftwf_import_wisdom_from_filename("pling.wisdom");
	UI ui(ringbuffer);

	ui.run();

	fftwf_export_wisdom_to_filename("pling.wisdom");
	SDL_free(pref_path);
	SDL_Quit();
}
