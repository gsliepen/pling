/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <SDL2/SDL.h>

#include "oscilloscope.hpp"
#include "spectrum.hpp"
#include "text.hpp"

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

	Text::Manager text_manager;
	Text::Widget text{text_manager};
	Oscilloscope::Widget oscilloscope;
	Spectrum::Widget spectrum;

	void resize(int w, int h);
	void process_window_event(const SDL_WindowEvent &ev);
	bool process_events();
	void render();

	public:
	UI(Oscilloscope::Signal &osc_signal, Spectrum::Signal &spectrum_signal);
	~UI();
	void run();
};
