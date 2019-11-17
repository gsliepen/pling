/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <SDL2/SDL.h>

#include "widgets/oscilloscope.hpp"
#include "widgets/spectrum.hpp"

class UI {
	int w = 800;
	int h = 480;
	bool paused{};

	struct Window {
		SDL_Window *window{};
		SDL_GLContext gl_context;

		Window(int w, int h);
		~Window();
	} window{w, h};

	Widgets::Oscilloscope oscilloscope;
	Widgets::Spectrum spectrum;

	void resize(int w, int h);
	void process_window_event(const SDL_WindowEvent &ev);
	bool process_events();
	void build();
	void render();

	public:
	UI(RingBuffer &signal);
	~UI();
	void run();
};
