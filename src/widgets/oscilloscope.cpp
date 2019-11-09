/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "oscilloscope.hpp"

#include <cassert>
#include <cstring>
#include <glm/glm.hpp>
#include <stdexcept>

#include "shader.hpp"

namespace Oscilloscope {

Signal::Signal() {
	samples.resize(2048);
}

Signal::~Signal() {
}

void Signal::add(const Chunk &chunk, float zero_crossing) {
	assert(pos + chunk.samples.size() <= samples.size());

	for(size_t i = 0; i < chunk.samples.size(); ++i) {
		float val = glm::clamp(chunk.samples[i] * 128.f, -128.f, 127.f) + 128;
		samples[pos++] = lrint(val);
	}
	best_crossing = zero_crossing + pos;

	pos %= samples.size();
}

static const GLchar *vertex_shader_source = R"(
#version 100

attribute vec4 coord;
varying vec2 texpos1;
varying vec2 texpos2;
uniform float dx;

void main(void) {
	gl_Position = vec4(coord.xy, 0.0, 1.0);
	texpos1 = coord.zw;
	texpos2 = coord.zw - vec2(dx, 0.0);
}
)";

static const GLchar *fragment_shader_source = R"(
#version 100
precision mediump float;

varying vec2 texpos1;
varying vec2 texpos2;
uniform sampler2D tex;
uniform float beam_width;

void main(void) {
	float val1 = texture2D(tex, texpos1).r;
	float val2 = texture2D(tex, texpos2).r;
	float minval = min(val1, val2);
	float maxval = max(val1, val2);

	float intensity = smoothstep(minval - beam_width, minval, texpos1.y) * smoothstep(maxval + beam_width, maxval, texpos1.y);
	intensity /= 1.0 + (maxval - minval) / beam_width;

	gl_FragColor = vec4(intensity, intensity + 0.125, intensity, 1.0);
}
)";

Widget::Widget(const Signal &signal): signal(signal), program(vertex_shader_source, fragment_shader_source) {
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, signal.get_samples().size(), 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, signal.get_samples().data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	attrib_coord = program.get_attrib("coord");
	uniform_tex = program.get_uniform("tex");
	uniform_beam_width = program.get_uniform("beam_width");
	uniform_dx = program.get_uniform("dx");
}

Widget::~Widget() {
	glDeleteTextures(1, &texture);
}

void Widget::set_position(float x, float y, float w, float h) {
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
}

void Widget::render(int screen_w, int screen_h) {
	const float scale_x = 2.0 / screen_w;
	const float scale_y = 2.0 / screen_h;

	program.use();
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glEnableVertexAttribArray(attrib_coord);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_tex, 0);
	glUniform1f(uniform_beam_width, 2.0 / h);

	/* Align such that we have a zero phase crossing at the center */
	const auto crossing = signal.get_crossing();
	const auto &samples = signal.get_samples();
	const auto signal_width = samples.size();

	const float center = crossing / signal_width;
	const float left = center - 0.5 * texture_w / signal_width;
	const float right = center + 0.5 * texture_w / signal_width;

	/* Update only the visible part of the texture */
	glUniform1f(uniform_dx, 1.0 * texture_w / signal_width / w);
	glBindTexture(GL_TEXTURE_2D, texture);
	const size_t visible_width = texture_w + 32;
	const size_t visible_offset = (lrintf(crossing - visible_width / 2) % signal_width) & ~7;

	if (visible_offset + visible_width <= signal_width) {
		glTexSubImage2D(GL_TEXTURE_2D, 0, visible_offset, 0, visible_width, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, samples.data() + visible_offset);
	} else {
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, visible_width + visible_offset - signal_width, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, samples.data());
		glTexSubImage2D(GL_TEXTURE_2D, 0, visible_offset, 0, signal_width - visible_offset, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, samples.data() + visible_offset);
	}

	/* Draw the oscilloscope */
	glm::vec4 rect[4] = {
		{x * scale_x - 1,       1 - y * scale_y,       left, 1},
		{(x + w) * scale_x - 1, 1 - y * scale_y,       right, 1},
		{x * scale_x - 1,       1 - (y + h) * scale_y, left, 0},
		{(x + w) * scale_x - 1, 1 - (y + h) * scale_y, right, 0},
	};

	glVertexAttribPointer(attrib_coord, 4, GL_FLOAT, GL_FALSE, 0, rect);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}
