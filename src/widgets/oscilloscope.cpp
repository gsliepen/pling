/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "oscilloscope.hpp"

#include <cassert>
#include <cstring>
#include <glm/glm.hpp>
#include <stdexcept>

#include "../shader.hpp"

namespace Widgets {

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

Oscilloscope::Oscilloscope(const RingBuffer &ringbuffer): ringbuffer(ringbuffer), program(vertex_shader_source, fragment_shader_source) {
	scope.resize(768);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, scope.size(), 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	attrib_coord = program.get_attrib("coord");
	uniform_tex = program.get_uniform("tex");
	uniform_beam_width = program.get_uniform("beam_width");
	uniform_dx = program.get_uniform("dx");
}

Oscilloscope::~Oscilloscope() {
	glDeleteTextures(1, &texture);
}

void Oscilloscope::render(int screen_w, int screen_h) {
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
	auto crossing = ringbuffer.get_crossing();
	const auto &samples = ringbuffer.get_samples();

	if (crossing < 0)
		crossing += samples.size();

	size_t crossing_sample = lrintf(crossing);
	float nudge = (crossing - crossing_sample) / scope.size();

	size_t pos = crossing_sample - scope.size() / 2;

	for (size_t i = 0; i < scope.size(); ++i) {
		const float value = samples[pos++ % samples.size()] * 127.0f + 128.0f;
		scope[i] = lrintf(glm::clamp(value, 0.0f, 255.0f));
	}

	/* Draw the oscilloscope */
	glUniform1f(uniform_dx, 1.0 / w);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, scope.size(), 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, scope.data());

	glm::vec4 rect[4] = {
		{x * scale_x - 1,       1 - y * scale_y,       0 + nudge, 1},
		{(x + w) * scale_x - 1, 1 - y * scale_y,       1 + nudge, 1},
		{x * scale_x - 1,       1 - (y + h) * scale_y, 0 + nudge, 0},
		{(x + w) * scale_x - 1, 1 - (y + h) * scale_y, 1 + nudge, 0},
	};

	glVertexAttribPointer(attrib_coord, 4, GL_FLOAT, GL_FALSE, 0, rect);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}
