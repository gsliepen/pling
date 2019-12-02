/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "ui.hpp"

#include "imgui/imgui.h"
#include "midi.hpp"
#include "state.hpp"

#include <fmt/format.h>
#include <map>

// Simple MIDI command -> function mapping, just for demonstration purposes.
struct Command {
	const MIDI::Port *port;
	uint8_t command;
	uint8_t number;

	bool operator <(const Command &other) const {
		if (port < other.port)
			return true;
		else if (port > other.port)
			return false;
		if (command < other.command)
			return true;
		else if (command > other.command)
			return false;
		return number < other.number;
	}
};

struct Function {
	enum class Type {
		Unassigned = 0,
		Button,
		Fader,
		Pot,
		Pad,
		Launch,
		Other,
	} type{};
	int row{};
	int column{};
	int other{};
};

static std::map<Command, Function> mapping;

static std::string port_to_string(int id, const MIDI::Port *port) {
	if (!port) {
		return "Select port...";
	}

	if (id == -1)
		return fmt::format("*: {}{}", port->get_name(), port->is_open() ? "" : " (not connected)");
	else
		return fmt::format("{}: {}{}", id + 1, port->get_name(), port->is_open() ? "" : " (not connected)");
}

void UI::build_learn_window() {
	ImGui::SetNextWindowPos({16.0f, 16.0f});
	ImGui::SetNextWindowSize({w - 32.0f, h - 32.0f});
	if (!ImGui::Begin("Learn", &show_learn_window, ImGuiWindowFlags_NoSavedSettings)) {
		ImGui::End();
		return;
	}

	ImGui::PushFont(big_font);

	static int filter_port_id = -1;
	static const MIDI::Port *filter_port{};

	auto &ports = MIDI::manager.get_ports();
	const MIDI::Port *port = filter_port_id >= 0 ? &ports[filter_port_id] : MIDI::manager.get_last_active_port();

	if (ImGui::BeginCombo("Controller", port_to_string(filter_port_id, port).c_str())) {
		if (ImGui::Selectable("Any controller", !filter_port)) {
			filter_port_id = -1;
			filter_port = {};
		}

		for (size_t id = 0; id < ports.size(); ++id) {
			if (ImGui::Selectable(port_to_string(id, &ports[id]).c_str(), filter_port_id == (int)id)) {
				filter_port_id = id;
				filter_port = &ports[id];
			}
		}

		ImGui::EndCombo();
	}

	std::vector<uint8_t> last_command;
	std::string description;
	if (port) {
		last_command = port->get_last_command();
		description = MIDI::command_to_text(last_command);
	}

	Command command{};

	ImGui::LabelText("MIDI command", description.c_str());

	if (last_command.size() == 3) {
		switch (last_command[0] & 0xf0) {
		case 0x80:
			// Map note-off to note-on
			last_command[0] = 0x90;
			[[fallthrough]];
		case 0x90:
		case 0xb0:
			command.port = port;
			command.command = last_command[0];
			command.number = last_command[1];
			break;
		default:
			// We don't handle this kind of event
			break;
		}
	}

	if (command.command) {
		static Function prev_function;
		auto &function = mapping[command];

		// Automatically learn the next unassigned control
		if (function.type == Function::Type::Unassigned) {
			function = prev_function;
			switch(function.type) {
			case Function::Type::Button:
			case Function::Type::Fader:
			case Function::Type::Pot:
			case Function::Type::Launch:
				function.column++;
				if (function.column > 16) {
					function.column = 1;
					function.row++;
					if (function.row > 16)
						function.row = 1;
				}
				break;
			case Function::Type::Other:
				function.other++;
				break;
			default:
				break;
			}
		}

		ImGui::BeginTabBar("learn_type");

		if (ImGui::BeginTabItem("Button")) {
			function.type = Function::Type::Button;
			ImGui::SliderInt("Row", &function.row, 1, 16);
			ImGui::SliderInt("Column", &function.column, 1, 16);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Fader")) {
			function.type = Function::Type::Fader;
			ImGui::SliderInt("Row", &function.row, 1, 16);
			ImGui::SliderInt("Column", &function.column, 1, 16);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Pot")) {
			function.type = Function::Type::Pot;
			ImGui::SliderInt("Row", &function.row, 1, 16);
			ImGui::SliderInt("Column", &function.column, 1, 16);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Pad")) {
			function.type = Function::Type::Pad;
			ImGui::SliderInt("Row", &function.row, 1, 16);
			ImGui::SliderInt("Column", &function.column, 1, 16);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Launch")) {
			function.type = Function::Type::Launch;
			ImGui::SliderInt("Row", &function.row, 1, 16);
			ImGui::SliderInt("Column", &function.column, 1, 16);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Other")) {
			function.type = Function::Type::Other;
			ImGui::Text("Transport:");
			ImGui::RadioButton("Loop", &function.other, 1);
			ImGui::SameLine();
			ImGui::RadioButton("Rewind", &function.other, 2);
			ImGui::SameLine();
			ImGui::RadioButton("Forward", &function.other, 3);
			ImGui::SameLine();
			ImGui::RadioButton("Stop", &function.other, 4);
			ImGui::SameLine();
			ImGui::RadioButton("Play", &function.other, 5);
			ImGui::SameLine();
			ImGui::RadioButton("Record", &function.other, 6);
			ImGui::Text("Track:");
			ImGui::RadioButton("Track-", &function.other, 7);
			ImGui::SameLine();
			ImGui::RadioButton("Track+", &function.other, 8);
			ImGui::SameLine();
			ImGui::RadioButton("Patch-", &function.other, 9);
			ImGui::SameLine();
			ImGui::RadioButton("Patch+", &function.other, 10);
			ImGui::Text("Bank:");
			ImGui::RadioButton("Mixer", &function.other, 11);
			ImGui::SameLine();
			ImGui::RadioButton("Instr", &function.other, 12);
			ImGui::SameLine();
			ImGui::RadioButton("Preset", &function.other, 13);
			ImGui::SameLine();
			ImGui::RadioButton("Clips", &function.other, 14);
			ImGui::SameLine();
			ImGui::RadioButton("Scenes", &function.other, 15);
			ImGui::Text("Modifiers:");
			ImGui::RadioButton("Shift", &function.other, 16);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Unassigned")) {
			function.type = Function::Type::Unassigned;
			ImGui::TextWrapped("The MIDI control you last pressed is not assigned to anything.\nSelect one of the control types to assign it.");
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();

		prev_function = function;
	}

	ImGui::PopFont();

	ImGui::End();

	if (!show_learn_window)
		state.set_learn_midi(false);
}
