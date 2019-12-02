/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <complex>
#include <fftw3.h>
#include <SDL2/SDL_opengles2.h>
#include <vector>

#include "../imgui/imgui.h"
#include "../pling.hpp"
#include "../shader.hpp"
#include "../utils.hpp"

namespace Widgets {

class Spectrum {
	static const size_t texture_size = 768;
	GLuint texture;

	const RingBuffer &ringbuffer;
	ShaderProgram program;
	GLint attrib_coord;
	GLint uniform_tex;
	GLint uniform_beam_width;
	GLint uniform_dx;

	int screen_w;
	int screen_h;

	float x;
	float y;
	float w;
	float h;

	static constexpr size_t fft_size = 8192;
	static constexpr float min_freq = key_to_frequency(-0.5f);
	static constexpr float max_freq = key_to_frequency(127.5f);

	fftwf_plan plan;
	std::vector<float> input;
	std::vector<float> window;
	std::vector<uint8_t> spectrum;

	static void render_callback(const ImDrawList* parent_list, const ImDrawCmd* cmd);

	public:
	Spectrum(const RingBuffer &ringbuffer);
	virtual ~Spectrum() final;
	virtual void render(int screen_w, int screen_h) final;
	virtual void build(int screen_w, int screen_h) final;
};

}
