/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <complex>
#include <fftw3.h>
#include <SDL2/SDL_opengles2.h>
#include <vector>

#include "pling.hpp"
#include "shader.hpp"

namespace Spectrum {

class Signal {
	size_t pos{};
	std::atomic<size_t> tail{};
	std::vector<float> samples;

	public:
	Signal();
	~Signal();

	void add(const Chunk &chunk);

	const std::vector<float> &get_samples() const {
		return samples;
	}

	size_t get_tail() const {
		return tail;
	}
};

class Widget {
	GLuint texture;

	float x{};
	float y{};
	float w{};
	float h{};

	const Signal &signal;
	ShaderProgram program;
	GLint attrib_coord;
	GLint uniform_tex;
	GLint uniform_beam_width;
	GLint uniform_dx;

	static const size_t fft_size = 8192;
	fftwf_plan plan;
	std::vector<float> input;
	std::vector<float> window;
	std::vector<uint8_t> spectrum;

	public:
	Widget(const Signal &signal);
	~Widget();
	void render(int screen_w, int screen_h);
	void set_position(float x, float y, float w, float h);
};

}
