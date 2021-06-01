/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <atomic>
#include <SDL2/SDL_opengles2.h>
#include <vector>

#include "../imgui/imgui.h"
#include "../pling.hpp"
#include "../shader.hpp"

namespace Widgets
{

class Oscilloscope
{
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

	std::vector<uint8_t> scope;
	static void render_callback(const ImDrawList *parent_list, const ImDrawCmd *cmd);

public:
	Oscilloscope(const RingBuffer &ringbuffer);
	~Oscilloscope();
	void render(int screen_w, int screen_h);
	void build(int screen_w, int screen_h);
};

}
