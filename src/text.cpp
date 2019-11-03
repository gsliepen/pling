/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "text.hpp"

#include <glm/glm.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

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

Font::Manager::Manager() {
	TTF_Init();
}

Font::Manager::~Manager() {
	fonts.clear();
	TTF_Quit();
}

Font *Font::Manager::get(const std::string &name, int size) {
	const auto key = std::make_pair(name, size);

	if (auto it = fonts.find(key); it != fonts.end())
		return &it->second;
	else
		return &fonts.emplace(key, Font(name, size)).first->second;
}

SDL_Surface *Font::render(const std::string &text, SDL_Color color) {
	return TTF_RenderUTF8_Blended(font, text.c_str(), color);
}

Text::Text() {
}

Text::~Text() {
}

void Text::set_font(Font::Manager &manager, const std::string &name, int size) {
	font = manager.get(name, size);
	dirty = true;
	glGenTextures(1, &texture);
}

void Text::set_text(const std::string &text) {
	this->text = text;
	dirty = true;
}

void Text::set_position(float x, float y, float align_x, float align_y) {
	this->x = x;
	this->y = y;
}

void Text::set_color(SDL_Color color) {
	this->color = color;
	dirty = true;
}

void Text::render() {
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

	glm::vec4 rect[4] = {
		{x * scale_x - 1,       1 - y * scale_y,       0, 0},
		{(x + w) * scale_x - 1, 1 - y * scale_y,       1, 0},
		{x * scale_x - 1,       1 - (y + h) * scale_y, 0, 1},
		{(x + w) * scale_x - 1, 1 - (y + h) * scale_y, 1, 1},
	};

	glVertexAttribPointer(attrib_coord, 4, GL_FLOAT, GL_FALSE, 0, rect);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
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

GLuint Text::program;
float Text::scale_x;
float Text::scale_y;
GLint Text::attrib_coord;
GLint Text::uniform_tex;

void Text::init() {
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, nullptr);
	glCompileShader(vertex_shader);

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, nullptr);
	glCompileShader(fragment_shader);

	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	attrib_coord = glGetAttribLocation(program, "coord");
	uniform_tex = glGetUniformLocation(program, "tex");
}

void Text::prepare_render(int w, int h) {
	scale_x = 2.0 / w;
	scale_y = 2.0 / h;

	glUseProgram(program);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnableVertexAttribArray(attrib_coord);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(uniform_tex, 0);
}
