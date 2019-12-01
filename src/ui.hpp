/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <SDL2/SDL.h>

#include "imgui/imgui.h"
#include "widgets/oscilloscope.hpp"
#include "widgets/spectrum.hpp"

class UI {
	float w = 800;
	float h = 480;
	bool paused{};

	struct Window {
		SDL_Window *window{};
		SDL_GLContext gl_context;

		Window(float w, float h);
		~Window();
	} window{w, h};

	RingBuffer &ringbuffer;
	Widgets::Oscilloscope oscilloscope;
	Widgets::Spectrum spectrum;

	ImFont *normal_font{};
	ImFont *big_font{};

	void resize(int w, int h);
	void process_window_event(const SDL_WindowEvent &ev);
	bool process_events();
	void build_status_bar();
	void build_volume_meter(const char *name);
	void build_volume_meters();
	void build_key_bar();
	void build_main_program();
	void build_buttons();
	void build();
	void render();

	public:
	UI(RingBuffer &ringbuffer);
	~UI();
	void run();
};
