/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "text.hpp"

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

namespace Text {

Font::Font(const std::string &name, int size) {
	font = TTF_OpenFont(name.c_str(), size);

	if (!font)
		throw std::runtime_error(TTF_GetError());
}

Font::Font(Font &&other)
{
	font = other.font;
	other.font = nullptr;
}

Font::~Font() {
}

static const GLchar *vertex_shader_source = R"(
#version 100

attribute vec4 coord;
varying vec2 texpos;

void main(void) {
	gl_Position = vec4(coord.xy, 0.0, 1.0);
	texpos = coord.zw;
}
)";

static const GLchar *fragment_shader_source = R"(
#version 100
precision mediump float;

varying vec2 texpos;
uniform sampler2D tex;

void main(void) {
	gl_FragColor = texture2D(tex, texpos);
}
)";


Manager::Manager(): program(vertex_shader_source, fragment_shader_source) {
	attrib_coord = program.get_attrib("coord");
	uniform_tex = program.get_uniform("tex");

	TTF_Init();
}

Manager::~Manager() {
	fonts.clear();
	TTF_Quit();
}

Font *Manager::get_font(const std::string &name, int size) {
	const auto key = std::make_pair(name, size);

	if (auto it = fonts.find(key); it != fonts.end())
		return &it->second;
	else
		return &fonts.emplace(key, Font(name, size)).first->second;
}

void Manager::prepare_render(int screen_w, int screen_h) {
	program.use();
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableVertexAttribArray(attrib_coord);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_tex, 0);

	scale = glm::vec2(2.0 / screen_w, 2.0 / screen_h);
}

SDL_Surface *Font::render(const std::string &text, SDL_Color color) {
	return TTF_RenderUTF8_Blended(font, text.c_str(), color);
}

Widget::Widget(Manager &manager): manager(manager) {
}

Widget::~Widget() {
}

void Widget::set_font(const std::string &name, int size) {
	font = manager.get_font(name, size);
	dirty = true;
	glGenTextures(1, &texture);
}

void Widget::set_text(const std::string &text) {
	this->text = text;
	dirty = true;
}

void Widget::set_position(float x, float y, float align_x, float align_y) {
	this->x = x;
	this->y = y;
}

void Widget::set_color(SDL_Color color) {
	this->color = color;
	dirty = true;
}

void Widget::render() {
	if (!font)
		return;

	glBindTexture(GL_TEXTURE_2D, texture);

	if (dirty) {
		auto surface = font->render(text.c_str(), color);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		w = surface->w;
		h = surface->h;
		dirty = false;

		SDL_FreeSurface(surface);
	}

	const auto scale = manager.get_scale();

	glm::vec4 rect[4] = {
		{x * scale.x - 1,       1 - y * scale.y,       0, 0},
		{(x + w) * scale.x - 1, 1 - y * scale.y,       1, 0},
		{x * scale.x - 1,       1 - (y + h) * scale.y, 0, 1},
		{(x + w) * scale.x - 1, 1 - (y + h) * scale.y, 1, 1},
	};

	glVertexAttribPointer(manager.get_attrib_coord(), 4, GL_FLOAT, GL_FALSE, 0, rect);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

}
