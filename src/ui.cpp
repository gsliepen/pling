/* SPDX-License-Identifier: GPL-3.0-or-later */


#include "ui.hpp"

#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_sdl.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

UI::Window::Window(int w, int h) {
	// Create the main SDL window
	uint32_t window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;

	SDL_DisplayMode desktop_mode;
	SDL_GetDesktopDisplayMode(0, &desktop_mode);

	if(desktop_mode.w == w && desktop_mode.h == h)
		window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	window = SDL_CreateWindow("Pling", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, window_flags);

	if (!window)
		throw std::runtime_error("Unable to create SDL window");

	SDL_SetWindowMinimumSize(window, w, h);

	// Get an OpenGL ES 2.0 context
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetSwapInterval(1);
	gl_context = SDL_GL_CreateContext(window);

	if (!gl_context)
		throw std::runtime_error("Unable to create OpenGL ES 2.0 context");
}

UI::Window::~Window() {
	if (gl_context) {
		SDL_GL_DeleteContext(gl_context);
	}

	if (window) {
		SDL_DestroyWindow(window);
	}
}

UI::UI(RingBuffer &signal): oscilloscope(signal), spectrum(signal) {
	// Initialize IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window.window, window.gl_context);
	ImGui_ImplOpenGL3_Init("#version 100");

	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	resize(w, h);

	/* Early write out of FFTW wisdom */
	fftwf_export_wisdom_to_filename("pling.wisdom");
}

UI::~UI() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void UI::resize(int w, int h) {
	this->w = w;
	this->h = h;
	glViewport(0, 0, w, h);

	/* Reposition the widgets */
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
		ImGui_ImplSDL2_ProcessEvent(&ev);

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

void UI::build() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window.window);
	ImGui::NewFrame();

	ImGui::Begin("Pling!");
	{
		ImGui::Text("Pling pling!!!");
		if(ImGui::Button("Button"))
			abort();
	}
	ImGui::End();
	ImGui::Render();
}

void UI::render() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	oscilloscope.render(w, h);
	spectrum.render(w, h);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window.window);
}


void UI::run() {
	while (process_events()) {
		build();

		if (!paused)
			render();
		else
			usleep(1000);
	}
}
