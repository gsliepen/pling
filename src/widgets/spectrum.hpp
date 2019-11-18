/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <complex>
#include <fftw3.h>
#include <SDL2/SDL_opengles2.h>
#include <vector>

#include "../pling.hpp"
#include "../shader.hpp"
#include "../widget.hpp"

namespace Widgets {

class Spectrum: public Widget {
	GLuint texture;

	const RingBuffer &ringbuffer;
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
	Spectrum(const RingBuffer &ringbuffer);
	virtual ~Spectrum() final;
	virtual void render(int screen_w, int screen_h) final;
};

}
