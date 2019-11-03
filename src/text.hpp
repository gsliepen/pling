/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_opengles2.h>

class Font {
	public:
	class Manager {
		std::map<std::pair<std::string, int>, Font> fonts;

		public:
		Manager();
		~Manager();
		Font *get(const std::string &name, int size);
	};

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

class Text {
	GLuint texture;

	bool dirty;

	float x{};
	float y{};
	float w{};
	float h{};

	std::string text;
	Font *font{};
	SDL_Color color{255, 255, 255, 255};

	static GLuint program;
	static float scale_x;
	static float scale_y;
	static GLint attrib_coord;
	static GLint uniform_tex;

	public:
	Text();
	~Text();
	void set_font(Font::Manager &manager, const std::string &name, int size);
	void set_text(const std::string &new_text);
	void set_position(float x, float y, float align_x = 0, float align_y = 0);
	void set_color(SDL_Color color);
	void render();

	static void init();
	static void prepare_render(int w, int h);
};
