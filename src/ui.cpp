/* SPDX-License-Identifier: GPL-3.0-or-later */


#include "ui.hpp"

#include <glm/glm.hpp>
#include "imgui/imgui.h"
#include "imgui/examples/imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "state.hpp"

#include <fmt/format.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengles2.h>

UI::Window::Window(float w, float h) {
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
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4{0.5f, 1.0f, 0.5f, 0.5f});
	ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, {0.5f, 0.5f});
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {1, 1});

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 32);

	ImGui::GetStyle().AntiAliasedLines = false;

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
		}
	}

	return true;
}

void UI::build_status_bar() {
	ImGui::SetNextWindowPos({16.0f, 0.0f});
	ImGui::BeginChild("status", {w - 32.0f, 16.0f}, false);
	ImGui::Text("Pling!");
	ImGui::EndChild();
}

void UI::build_volume_meter(const char *name) {
	float dB = amplitude_to_dB(ringbuffer.get_rms() * state.get_master_volume());
	float master_dB = amplitude_to_dB(state.get_master_volume());

	ImGui::BeginChild(name, {16.0f, h}, false);

	// Draw the meter
	auto pos = ImGui::GetCursorScreenPos();
	auto list = ImGui::GetWindowDrawList();
	if (dB > max_dB) {
		// We're clipping for sure now.
		list->AddRectFilled({pos.x, pos.y + h}, {pos.x + 16.0f, pos.y}, ImColor{255, 0, 0, 255});
	} else if (dB > 0) {
		// We are in the 10 dB headroom range.
		list->AddRectFilled({pos.x, pos.y + h}, {pos.x + 16.0f, pos.y + h * 0.2f}, ImColor{0, 128, 0, 255});
		list->AddRectFilled({pos.x, pos.y + h * 0.2f}, {pos.x + 16.0f, pos.y + h * (max_dB - dB) / (max_dB - min_dB)}, ImColor{192, 192, 0, 255});
	} else {
		// We are in safe range.
		list->AddRectFilled({pos.x, pos.y + h}, {pos.x + 16.0f, pos.y + h * (max_dB - dB) / (max_dB - min_dB)}, ImColor{0, 128, 0, 255});
	}

	// Add a slider to allow control of the master volume
	if (ImGui::VSliderFloat(name, {16.0f, h}, &master_dB, min_dB, max_dB, name))
		state.set_master_volume(dB_to_amplitude(master_dB));

	// Draw 6 dB tick markers
	for (float dB = min_dB; dB < max_dB; dB += amplitude_to_dB(2.0f)) {
		const float y = h * (max_dB - dB) / (max_dB - min_dB);
		list->AddLine({pos.x, pos.y + y}, {pos.x + 16.0f, pos.y + y}, ImColor{255, 255, 255, lrintf(dB) == 0 ? 128 : 64});
	}

	ImGui::EndChild();
}

void UI::build_volume_meters() {
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{0, 0, 0, 0});
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4{0, 0, 0, 0});
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4{0, 0, 0, 0});

	ImGui::SetNextWindowPos({0.0f, 0.0f});
	build_volume_meter("L");

	ImGui::SetNextWindowPos({w - 16.0f, 0.0f});
	build_volume_meter("R");

	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
}

void UI::build_key_bar() {
	ImGui::SetNextWindowPos({0.0f, h - 16.0f});
	ImGui::BeginChild("keys", {1.0f * w, 16.0f}, false);
	auto draw_list = ImGui::GetWindowDrawList();
	auto pos = ImGui::GetCursorScreenPos();
	pos.x += 16.0f;
	auto &keys = state.get_keys();
	float key_size = (w - 32.0f) / 128.0f;
	auto bent_pos = pos;
	bent_pos.x += key_size * state.get_bend() / 4096.0f;

	draw_list->AddRectFilled({0, 0}, {w, 16}, ImColor(128, 0, 0, 128));
	for (uint8_t key = 0; key < 128; ++key) {
		// Draw octave dividers
		if (key && key % 12 == 0)
			draw_list->AddLine({pos.x + key_size * (key + 0.5f), pos.y}, {pos.x + key_size * (key + 0.5f), pos.y + 16.0f}, ImColor(255, 255, 255, 128));

		// Draw pressed keys
		if (keys[key])
			draw_list->AddRectFilled({bent_pos.x + key_size * key + 1.0f, pos.y + 16.0f}, {bent_pos.x + key_size * (key + 1), pos.y + 16.0f - keys[key] / 8.0f}, ImColor(255, 255, 255, 255));
	}

	ImGui::EndChild();
}

void UI::build_program_select() {
	auto [active_port, active_channel] = state.get_active_channel();

	if (!active_port) {
		show_program_select = false;
		return;
	}

	ImGui::SetNextWindowPos({16.0f, 16.0f});
	ImGui::SetNextWindowSize({w - 32.0f, h - 32.0f});
	if (!ImGui::Begin("Program selection", &show_program_select, ImGuiWindowFlags_NoSavedSettings) || ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE)) {
		show_program_select = false;
		ImGui::End();
		return;
	}

	auto &program = active_port->get_channel(active_channel).program;
	const auto current_MIDI_program = program->get_MIDI_program();

	ImGui::PushFont(big_font);
	ImGui::Columns(2);
	if (ImGui::BeginCombo("Port", active_port->get_name().c_str())) {
		for (auto &port: MIDI::manager.get_ports())
			if (ImGui::Selectable(port.get_name().c_str(), &port == active_port))
				state.set_active_channel(port, active_channel);
		ImGui::EndCombo();
	}

	ImGui::NextColumn();
	int selected_channel = active_channel + 1;
	if (ImGui::SliderInt("Channel", &selected_channel, 1, 16))
		state.set_active_channel(*active_port, selected_channel - 1);
	ImGui::Columns();
	ImGui::PopFont();

	ImGui::Separator();

	ImGui::BeginChild("Program list");
	ImGui::Columns(4);

	for (uint8_t n = 0; n < 128; ++n)
	{
		if (ImGui::Selectable(fmt::format("{:03d}: Program name", n + 1).c_str(), n == current_MIDI_program))
			MIDI::manager.change(active_port, active_channel, n);

		ImGui::NextColumn();
	}

	ImGui::Columns();
	ImGui::EndChild();

	ImGui::End();
}

void UI::build_main_program() {
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{0.0f, 1.0f, 1.0f, 0.1f});
	ImGui::Begin("Main program", {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);

	const auto &[port, channel] = state.get_active_channel();

	if (!port) {
		ImGui::PushFont(big_font);
		ImGui::Text("Connect a MIDI controller!");
		ImGui::PopFont();
		ImGui::End();
		ImGui::PopStyleColor();
		return;
	}

	if (ImGui::Selectable(fmt::format("Controller: {}  Channel: {:02d}", port->get_name(), channel + 1).c_str()))
		show_program_select = true;

	const auto &program = port->get_channel(channel).program;

	ImGui::PushFont(big_font);

	if(ImGui::Selectable(fmt::format("{:03d}: {}", program->get_MIDI_program() + 1, program->get_name()).c_str()) || ImGui::IsKeyPressed(SDL_SCANCODE_P))
		show_program_select = true;

	ImGui::PopFont();

	ImGui::Text(fmt::format("Synth engine: {}", program->get_engine_name()).c_str());
	ImGui::Separator();
	ImGui::Selectable("Track: 01  Pattern: 01  Beat: 4/4  Tempo: 120");
	ImGui::PushFont(big_font);
	ImGui::Selectable("00:00:00");
	ImGui::PopFont();
	ImGui::End();
	ImGui::PopStyleColor();
}

void UI::build_buttons() {
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4{1.0f, 0.0f, 1.0f, 0.1f});
	ImGui::Begin("Buttons", {}, (ImGuiWindowFlags_NoDecoration & ~ImGuiWindowFlags_NoTitleBar) | ImGuiWindowFlags_NoSavedSettings);
	auto size = ImGui::GetContentRegionAvail();
	ImVec2 button_size = {size.x / 3, size.y / 2};
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0.0f, 0.0f});
	ImGui::PushFont(big_font);
	if(ImGui::Button("Learn", button_size)) {
		state.set_learn_midi(true);
		MIDI::manager.panic();
		show_learn_window = true;
	}
	ImGui::SameLine();
	ImGui::Button("Load", button_size); ImGui::SameLine();
	ImGui::Button("Save", button_size);

	if (ImGui::Button("Panic", button_size))
		MIDI::manager.panic();

	ImGui::SameLine();
	ImGui::Button("Controls", button_size); ImGui::SameLine();
	ImGui::Button("Transport", button_size);
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::End();
	ImGui::PopStyleColor();
}

void UI::build() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window.window);
	ImGui::NewFrame();

	// Create a fullscreen window
	ImGui::SetNextWindowPos({0.0f, 0.0f});
	ImGui::SetNextWindowSize({float(w), float(h)});
	ImGui::Begin("fullscreen", {}, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground);

	// Add edge decorations
	build_status_bar();
	build_volume_meters();
	build_key_bar();

	// Use a 2x3 grid for the remaining area
	float gw = (w - 32.0f) / 2.0f;
	float gh = (h - 32.0f) / 3.0f;

	// Main program window
	ImGui::SetNextWindowPos({16.0f, 16.0f});
	ImGui::SetNextWindowSize({gw, gh});
	build_main_program();

	// Main buttons
	ImGui::SetNextWindowPos({16.0f + gw, 16.0f});
	ImGui::SetNextWindowSize({gw, gh});
	build_buttons();

	// Oscilloscope window
	ImGui::SetNextWindowPos({16.0f, 16.0f + gh * 1.0f});
	ImGui::SetNextWindowSize({gw * 2.0f, gh});
	oscilloscope.build(w, h);

	// Spectrum analyzer window
	ImGui::SetNextWindowPos({16.0f, 16.0f + gh * 2.0f});
	ImGui::SetNextWindowSize({gw * 2.0f, gh});
	spectrum.build(w, h);

	// Context sensitive widgets
	const auto &[port, channel] = state.get_active_channel();

	if (port) {
		if (auto cc = state.get_active_cc(); cc) {
			const auto &program = programs.get_last_activated_program();
			ImGui::SetNextWindowPos({16.0f, 16.0f + gh * 1.0f});
			ImGui::SetNextWindowSize({gw * 2.0f, gh});
			program->build_cc_widget(cc);

			if (auto last_change = state.get_last_active_cc_change(); decltype(last_change)::clock::now() - last_change > std::chrono::seconds(10))
				state.set_active_cc(0);
		}
	}

	ImGui::End();

	if (show_learn_window)
		build_learn_window();

	// Program select window
	if (show_program_select)
		build_program_select();

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
		render();
	}
}
