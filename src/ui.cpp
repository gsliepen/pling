/* SPDX-License-Identifier: GPL-3.0-or-later */


#include "ui.hpp"

#include <glm/glm.hpp>
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_sdl.h"
#include "imgui/examples/imgui_impl_opengl3.h"

#include <fmt/format.h>
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

UI::UI(RingBuffer &ringbuffer): ringbuffer(ringbuffer), oscilloscope(ringbuffer), spectrum(ringbuffer) {
	// Initialize IMGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window.window, window.gl_context);
	ImGui_ImplOpenGL3_Init("#version 100");

	ImGuiIO &io = ImGui::GetIO();
	normal_font = io.Fonts->AddFontFromFileTTF("../src/imgui/misc/fonts/DroidSans.ttf", 13.0f);
	big_font = io.Fonts->AddFontFromFileTTF("../src/imgui/misc/fonts/DroidSans.ttf", 26.0f);


	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4{0.5f, 1.0f, 0.5f, 0.5f});
	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, {1});

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

	const auto borderless = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
	const auto childflags = (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings;

	// Create a fullscreen window
	ImGui::SetNextWindowPos({0.0f, 0.0f});
	ImGui::SetNextWindowSize({float(w), float(h)});
	ImGui::Begin("fullscreen", {}, borderless);

	// Add manually positioned child windows for the 16 pixel edge decorations
	ImGui::SetNextWindowPos({16.0f, 0.0f});
	ImGui::BeginChild("top", {1.0f * w - 32.0f, 16.0f}, false);
	ImGui::Text("Pling!");
	ImGui::SameLine();
	ImGui::Text("100%%");
	ImGui::EndChild();

	float dB = 20 * log10f(ringbuffer.get_rms()); // dB

	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(glm::clamp(dB, 0.0f, 1.0f), 0.0f, 0.0f, 1.0f));

	ImGui::SetNextWindowPos({0.0f, 16.0f});
	ImGui::BeginChild("left", {16.0f, h - 32.0f}, false);
	ImGui::VSliderFloat("left amp", {16.0f, h - 32.0f}, &dB, -40.0f, 10.0f, "L");
	ImGui::EndChild();

	ImGui::SetNextWindowPos({w - 16.0f, 16.0f});
	ImGui::BeginChild("right", {16.0f, h - 32.0f}, false);
	ImGui::VSliderFloat("right amp", {16.0f, h - 32.0f}, &dB, -40.0f, 10.0f, "R");
	ImGui::EndChild();

	ImGui::PopStyleColor();

	ImGui::SetNextWindowPos({16.0f, h - 16.0f});
	ImGui::BeginChild("bottom", {1.0f * w - 32.0f, 16.0f}, false);
	ImGui::Text("Active keys");
	ImGui::SameLine();
	ImGui::Text("etc");
	ImGui::EndChild();

	// Use a 2x3 grid for the remaining area
	float gw = (w - 32.0f) / 2.0f;
	float gh = (h - 32.0f) / 3.0f;

	// Main program window
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.0f, 1.0f, 1.0f, 0.1f});
	ImGui::SetNextWindowPos({16.0f, 16.0f});
	ImGui::SetNextWindowSize({gw, gh});
	ImGui::Begin("Main program", {}, childflags);
	static const char *items[] = {
		"000: Accoustic Grand Piano",
		"001: Bright Accoustic Piano",
		"002: Electric Grand Piano",
		"...",
	};
	ImGui::Selectable("Controller: MIDI Controler 1  Channel: 01");
	ImGui::PushFont(big_font);
	static int cur = 0;
	if(ImGui::Selectable(items[cur]))
		ImGui::OpenPopup("Instrument");
	ImGui::PopFont();
	if(ImGui::BeginPopup("Instrument")) {
		ImGui::Text("Choose an instrument:");
		ImGui::Combo("instrument", &cur, items, 3);
		ImGui::EndPopup();
	}
	ImGui::Text("Synth: Simple");
	ImGui::Separator();
	ImGui::Selectable("Track: 01  Pattern: 01  Beat: 4/4  Tempo: 120");
	ImGui::PushFont(big_font);
	ImGui::Selectable("00:00:00");
	ImGui::PopFont();
	ImGui::End();
	ImGui::PopStyleColor();

	// Main buttons
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{1.0f, 0.0f, 1.0f, 0.1f});
	ImGui::SetNextWindowPos({16.0f + gw, 16.0f});
	ImGui::SetNextWindowSize({gw, gh});
	ImGui::Begin("Buttons", {}, childflags);
	auto size = ImGui::GetContentRegionAvail();
	ImVec2 button_size = {size.x / 3, size.y / 2};
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
	ImGui::PushFont(big_font);
	ImGui::Button("Learn", button_size); ImGui::SameLine();
	ImGui::Button("Load", button_size); ImGui::SameLine();
	ImGui::Button("Save", button_size);
	ImGui::Button("Panic", button_size); ImGui::SameLine();
	ImGui::Button("Controls", button_size); ImGui::SameLine();
	ImGui::Button("Transport", button_size);
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::End();
	ImGui::PopStyleColor();
	ImGui::End();

	// Oscilloscope window
	ImGui::SetNextWindowPos({16.0f, 16.0f + gh * 1.0f});
	ImGui::SetNextWindowSize({gw * 2.0f, gh});
	oscilloscope.build(w, h);

	// Spectrum analyzer window
	ImGui::SetNextWindowPos({16.0f, 16.0f + gh * 2.0f});
	ImGui::SetNextWindowSize({gw * 2.0f, gh});
	spectrum.build(w, h);

	ImGui::Render();
}

void UI::render() {
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
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
