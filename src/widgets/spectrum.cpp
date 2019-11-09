/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "spectrum.hpp"

#include <cassert>
#include <complex>
#include <cstring>
#include <fftw3.h>
#include <glm/glm.hpp>
#include <stdexcept>

#include "shader.hpp"

namespace Spectrum {

Signal::Signal() {
	samples.resize(16384);
}

Signal::~Signal() {
}

void Signal::add(const Chunk &chunk) {
	assert(pos + chunk.samples.size() <= samples.size());

	for(size_t i = 0; i < chunk.samples.size(); ++i) {
		samples[pos++] = chunk.samples[i];
	}

	pos %= samples.size();
	tail = pos;
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
	input.resize(fft_size + 2);
	window.resize(fft_size);
	spectrum.resize(768);
	plan = fftwf_plan_dft_r2c_1d(fft_size, input.data(), reinterpret_cast<fftwf_complex *>(input.data()), FFTW_MEASURE | FFTW_R2HC);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, spectrum.size(), 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, spectrum.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	attrib_coord = program.get_attrib("coord");
	uniform_tex = program.get_uniform("tex");
	uniform_beam_width = program.get_uniform("beam_width");
	uniform_dx = program.get_uniform("dx");

	/* Initialize a Hann window, see https://en.wikipedia.org/wiki/Hann_function */
	for (size_t i = 0; i < fft_size; ++i) {
		window[i] = 0.5 - 0.5 * cos(2 * M_PI * i / fft_size);
	}
}

Widget::~Widget() {
	glDeleteTextures(1, &texture);

	fftwf_destroy_plan(plan);
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

	/* Copy the latest audio data into the input buffer */
	const auto &samples = signal.get_samples();
	const size_t n = samples.size();
	size_t j = (signal.get_tail() - fft_size) % n;

	for (size_t i = 0; i < fft_size; ++i) {
		input[i] = window[i] * samples[(j++ % n)];
	}

	fftwf_execute(plan);

	const auto output = reinterpret_cast<std::complex<float> *>(input.data());

	/* Build a texture from the FFT result */
	const float low = 27.5; // frequency of lowest note
	const float range = 88.0 / 12.0; // octaves
	const float x1 = low / 24000.0;

	for (size_t i = 0; i < spectrum.size(); ++i) {
		/* Calculate texture x position of input and output */
		const float u_out = i / float(spectrum.size());
		const float u_in = x1 * exp2(u_out * range);
		const size_t i_in = u_in * (fft_size / 2);

		/* Interpolate between FFT samples */
		const float fr = u_in * (fft_size / 2) - i_in;
		const float value0 = log10(abs(output[i_in]) / fft_size * 4);
		const float value1 = log10(abs(output[i_in + 1]) / fft_size * 4);
		const float dB = 20 * glm::mix(value0, value1, fr);
		const float value = 1 + (dB - 10) / 60; // -40..10 dB range

		spectrum[i] = lrintf(glm::clamp(value * 255.0f, 0.0f, 255.0f));
	}

	/* Draw the spectrum */
	glUniform1f(uniform_dx, 1.0 / w);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, spectrum.size(), 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, spectrum.data());

	glm::vec4 rect[4] = {
		{x * scale_x - 1,       1 - y * scale_y,       0, 1},
		{(x + w) * scale_x - 1, 1 - y * scale_y,       1, 1},
		{x * scale_x - 1,       1 - (y + h) * scale_y, 0, 0},
		{(x + w) * scale_x - 1, 1 - (y + h) * scale_y, 1, 0},
	};

	glVertexAttribPointer(attrib_coord, 4, GL_FLOAT, GL_FALSE, 0, rect);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}
