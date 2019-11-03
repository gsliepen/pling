/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <SDL2/SDL_opengles2.h>
#include <vector>

#include "pling.hpp"
#include "shader.hpp"

namespace Oscilloscope {

class Signal {
	size_t pos{};
	std::atomic<float> best_crossing{};
	float prev_crossing{};
	std::vector<uint8_t> samples;

	public:
	Signal();
	~Signal();

	void add(const Chunk &chunk, float zero_crossing);

	float get_crossing() const {
		return best_crossing;
	}

	const std::vector<uint8_t> &get_samples() const {
		return samples;
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

	public:
	Widget(const Signal &signal);
	~Widget();
	void render(int screen_w, int screen_h);
	void set_position(float x, float y, float w, float h);
};

}
