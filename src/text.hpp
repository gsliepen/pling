/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengles2.h>

#include "shader.hpp"

namespace Text {

class Font {
	public:
	private:
	TTF_Font *font{};

	public:
	Font(const std::string &name, int size);
	~Font();
	Font(const Font &other) = delete;
	Font(Font &&other);
	Font &operator=(const Font &other) = delete;

	SDL_Surface *render(const std::string &text, SDL_Color color);
};

class Manager {
	std::map<std::pair<std::string, int>, Font> fonts;

	ShaderProgram program;
	float scale_x;
	float scale_y;
	GLint attrib_coord;
	GLint uniform_tex;

	glm::vec2 scale;

	public:
	Manager();
	~Manager();
	Font *get_font(const std::string &name, int size);
	void prepare_render(int screen_w, int screen_h);

	glm::vec2 get_scale() {
		return scale;
	};

	GLint get_attrib_coord() {
		return attrib_coord;
	}
};

class Widget {
	Manager &manager;
	GLuint texture;

	bool dirty;

	float x{};
	float y{};
	float w{};
	float h{};

	std::string text;
	Font *font{};
	SDL_Color color{255, 255, 255, 255};

	public:
	Widget(Manager &manager);
	~Widget();
	void set_font(const std::string &name, int size);
	void set_text(const std::string &new_text);
	void set_position(float x, float y, float align_x = 0, float align_y = 0);
	void set_color(SDL_Color color);
	void render();
};

}
