/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "ui.hpp"
#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

static const char *font_file = "/usr/share/fonts/truetype/freefont/FreeSans.ttf";

UI::Window::Window(int w, int h) {
	uint32_t window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	SDL_DisplayMode desktop_mode;
	SDL_GetDesktopDisplayMode(0, &desktop_mode);

	if(desktop_mode.w == w && desktop_mode.h == h)
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow("Pling", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, window_flags);
	SDL_SetWindowMinimumSize(window, w, h);

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1);
	gl_context = SDL_GL_CreateContext(window);
}

UI::Window::~Window() {
	if (gl_context) {
		SDL_GL_DeleteContext(gl_context);
	}

	if (window) {
		SDL_DestroyWindow(window);
	}
}

UI::UI(Oscilloscope::Signal &osc_signal, Spectrum::Signal &spectrum_signal): oscilloscope(osc_signal), spectrum(spectrum_signal) {
	text.set_text("Pling!");

	resize(w, h);

	/* Early write out of FFTW wisdom */
	fftwf_export_wisdom_to_filename("pling.wisdom");
}

UI::~UI() {
}

void UI::resize(int w, int h) {
	this->w = w;
	this->h = h;
	glViewport(0, 0, w, h);

	const float font_scale = h / 480.0;

	/* Reposition the widgets */
	text.set_font(font_file, 12 * font_scale);
	text.set_position(16, 16);
	spectrum.set_position(16, 1 * (h / 3) + 16, w - 32, h / 3 - 32);
	oscilloscope.set_position(16, 2 * (h / 3) + 16, w - 32, h / 3 - 32);
}

void UI::process_window_event(const SDL_WindowEvent &ev) {
	switch(ev.event) {
	case SDL_WINDOWEVENT_SIZE_CHANGED:
		resize(ev.data1, ev.data2);
		break;

	default:
		break;
	}
}

bool UI::process_events() {
	for (SDL_Event ev; SDL_PollEvent(&ev);) {
		switch (ev.type) {
		case SDL_QUIT:
			return false;

		case SDL_WINDOWEVENT:
			process_window_event(ev.window);
			break;

		case SDL_KEYDOWN:
			if (ev.key.keysym.sym == SDLK_p) {
				paused = !paused;
			}
			break;
		}
	}

	return true;
}

void UI::render() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	text_manager.prepare_render(w, h);
	text.render();
	oscilloscope.render(w, h);
	spectrum.render(w, h);
	SDL_GL_SwapWindow(window.window);
}


void UI::run() {
	while (process_events()) {
		if (!paused)
			render();
		else
			usleep(1000);
	}
}
