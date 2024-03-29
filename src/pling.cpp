/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "pling.hpp"

#include <chrono>
#include <fftw3.h>
#include <filesystem>
#include <fmt/ostream.h>
#include <glm/glm.hpp>
#include <iostream>
#include <SDL2/SDL.h>
#include <set>

#include "config.hpp"
#include "midi.hpp"
#include "program-manager.hpp"
#include "ui.hpp"
#include "state.hpp"
#include "widgets/oscilloscope.hpp"
#include "widgets/spectrum.hpp"

static RingBuffer ringbuffer{16384};
Program::Manager programs;
Config config;
float sample_rate = 48000;

static std::random_device random_device;
thread_local std::default_random_engine random_engine(random_device());

State state;
MIDI::Manager MIDI::manager(programs);

static void audio_callback(void *userdata, uint8_t *stream, int len)
{
	static Chunk chunk;

	/* Render samples from active programs */
	programs.render(chunk);

	/* Add delay effect */
	const int nsamples = len / 4;

	for (int i = 0; i < nsamples; ++i) {
		chunk.samples[i] += ringbuffer[i - 10000] * 0.25;
		chunk.samples[i] += ringbuffer[i - 10002] * -0.25;
	}

	/* Add the samples to the oscilloscope */
	ringbuffer.add(chunk, programs.get_zero_crossing(-384), programs.get_base_frequency());

	/* Convert to 16-bit signed stereo */
	int16_t *data = (int16_t *)stream;
	float amplitude = state.get_master_volume() * 0.25f; // leave ~12 dB headroom

	for (int i = 0; i < nsamples; i++) {
		*data++ = glm::clamp(chunk.samples[i] * amplitude, -1.f, 1.f) * 32767;
		*data++ = glm::clamp(chunk.samples[i] * amplitude, -1.f, 1.f) * 32767;
	}
}

static void setup_audio()
{
	SDL_AudioSpec want{}, have{};

	want.freq = config["sample_rate"].as<int>(48000);
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = chunk_size;
	want.callback = audio_callback;

	auto name = config["audio_device"].as<std::string>("");

	SDL_AudioDeviceID dev = SDL_OpenAudioDevice(name.c_str(), 0, &want, &have, 0);

	if (!dev) {
		fmt::print(std::cerr, "Could not open {}\n", name);
		dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	}

	if (!dev) {
		throw std::runtime_error(SDL_GetError());
	}

	if (have.samples != chunk_size) {
		throw std::runtime_error("Could not get requested audio chunk size");
	}

	sample_rate = have.freq;
	std::cerr << sample_rate << "\n";

	SDL_PauseAudioDevice(dev, 0);
}

static void benchmark()
{
	static Chunk chunk;

	std::shared_ptr<Program> program;
	programs.change(program, 5);
	programs.activate(program);

	// Warm-up
	program->release_all();

	for (size_t i = 0; i < 32; ++i) {
		program->note_on(48 + i, 80 + i);
	}

	for (size_t i = 0; i < 100; ++i) {
		programs.render(chunk);
	}

	// Measurement
	program->release_all();

	for (size_t i = 0; i < 32; ++i) {
		program->note_on(48 + i, 80 + i);
	}

	using clock = std::chrono::steady_clock;
	auto begin = clock::now();

	for (size_t i = 0; i < 10000; ++i) {
		programs.render(chunk);
	}

	auto end = clock::now();
	auto diff = end - begin;

	std::cout << "Rendered 1000 chunks in " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count() << "ms\n";
}

int main(int argc, char *argv[])
{
	if (argc > 1 && std::string(argv[1]) == "benchmark") {
		benchmark();
		return 0;
	}

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	auto pref_path = SDL_GetPrefPath(NULL, "pling");
	config.init(pref_path);
	SDL_free(pref_path);

	setup_audio();
	MIDI::manager.start();

	fftwf_import_wisdom_from_filename(config.get_cache_path("fft.wisdom").c_str());
	UI ui(ringbuffer);

	ui.run();

	fftwf_export_wisdom_to_filename(config.get_cache_path("fft.wisdom").c_str());
	SDL_Quit();
}
