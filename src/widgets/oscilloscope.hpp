/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <SDL2/SDL_opengles2.h>
#include <vector>

#include "../pling.hpp"
#include "../shader.hpp"
#include "../widget.hpp"

namespace Widgets {

class Oscilloscope: public Widget {
	GLuint texture;

	const RingBuffer &signal;
	ShaderProgram program;
	GLint attrib_coord;
	GLint uniform_tex;
	GLint uniform_beam_width;
	GLint uniform_dx;

	std::vector<uint8_t> scope;

	public:
	Oscilloscope(const RingBuffer &signal);
	virtual ~Oscilloscope() final;
	virtual void render(int screen_w, int screen_h) final;
};

}
