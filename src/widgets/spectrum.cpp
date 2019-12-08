/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "spectrum.hpp"

#include <cassert>
#include <complex>
#include <cstring>
#include <fftw3.h>
#include <fmt/format.h>
#include <glm/glm.hpp>
#include <stdexcept>

#include "shader.hpp"
#include "../utils.hpp"

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

	gl_FragColor = vec4(intensity, intensity, intensity + 0.125, 1.0);
}
)";

Spectrum::Spectrum(const RingBuffer &ringbuffer): ringbuffer(ringbuffer), program(vertex_shader_source, fragment_shader_source) {
	input.resize(fft_size + 2);
	window.resize(fft_size);
	spectrum.resize(texture_size);
	plan = fftwf_plan_dft_r2c_1d(fft_size, input.data(), reinterpret_cast<fftwf_complex *>(input.data()), FFTW_MEASURE | FFTW_R2HC);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, spectrum.size(), 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
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

Spectrum::~Spectrum() {
	glDeleteTextures(1, &texture);

	fftwf_destroy_plan(plan);
}

void Spectrum::render(int screen_w, int screen_h) {
	const float scale_x = 2.0 / screen_w;
	const float scale_y = 2.0 / screen_h;

	program.use();
	glDisable(GL_BLEND);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glEnableVertexAttribArray(attrib_coord);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_tex, 0);
	glUniform1f(uniform_beam_width, 2.0 / h);

	const auto output = reinterpret_cast<std::complex<float> *>(input.data());

	/* Build a texture from the FFT result */
	const float range = log2(max_freq / min_freq);
	const float x1 = min_freq / (sample_rate / 2);

	for (size_t i = 0; i < spectrum.size(); ++i) {
		/* Calculate texture x position of input and output */
		const float u_out = i / float(spectrum.size());
		const float u_in = x1 * exp2(u_out * range);
		const size_t i_in = u_in * (fft_size / 2);

		/* Interpolate between FFT samples */
		const float fr = u_in * (fft_size / 2) - i_in;
		const float value0 = amplitude_to_dB(abs(output[i_in]) / fft_size * 4);
		const float value1 = amplitude_to_dB(abs(output[i_in + 1]) / fft_size * 4);
		const float dB = glm::mix(value0, value1, fr);
		const float value = 1 + (dB - max_dB) / (max_dB - min_dB);

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

void Spectrum::render_callback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
	auto this_ = (Spectrum *)cmd->UserCallbackData;
	this_->render(this_->screen_w, this_->screen_h);
}

void Spectrum::build(int screen_w, int screen_h) {
	/* Copy the latest audio data into the input buffer */
	const auto &samples = ringbuffer.get_samples();
	const size_t n = samples.size();
	size_t j = (ringbuffer.get_tail() - fft_size) % n;

	for (size_t i = 0; i < fft_size; ++i) {
		input[i] = window[i] * samples[(j++ % n)];
	}

	fftwf_execute(plan);

	ImGui::Begin("Spectrum analyzer", nullptr, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

	/* Remember the window position and size for the callback */
	auto widget_pos = ImGui::GetCursorScreenPos();
	auto region_min = ImGui::GetWindowContentRegionMin();
	auto region_max = ImGui::GetWindowContentRegionMax();

	x = widget_pos.x;
	y = widget_pos.y;
	w = region_max.x - region_min.x;
	h = region_max.y - region_min.y;

	this->screen_w = screen_w;
	this->screen_h = screen_h;

	/* Draw the spectrum curve */
	auto list = ImGui::GetWindowDrawList();
	list->AddCallback(render_callback, this);
	list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);

	/* Draw the grid overlay */
	for (float dB = min_dB; dB < max_dB + 0.1f; dB += amplitude_to_dB(4.0f)) {
		float dBy = (max_dB - dB) / (max_dB - min_dB) * h;
		if (dB < lrintf(max_dB))
			list->AddLine({x, y + dBy}, {x + w, y + dBy}, ImColor(255, 255, 255, dB == 0 ? 64 : 32));
		list->AddText({x, y + dBy}, ImColor(255, 255, 255, dB == 0 ? 128 : 64), fmt::format("{:+3.0f} dB", dB).c_str());
	}

	float key_size = w / 128.0f;
	for (int key = 12; key < 128; key += 12) {
		list->AddLine({x + key_size * (key + 0.5f), y}, {x + key_size * (key + 0.5f), y + h}, ImColor(255, 255, 255, key == 60 ? 64 : 32));
		float freq = key_to_frequency(key);
		std::string text;

		if (freq < 1e3f)
			text = fmt::format("{:.1f} Hz", freq);
		else
			text = fmt::format("{:.1f} kHz", freq / 1e3f);

		list->AddText({x + 2 + key_size * (key + 0.5f), y}, ImColor(255, 255, 255, key == 60 ? 128 : 64), text.c_str());
	}

	ImGui::End();
}

}
