/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "controller.hpp"

#include <charconv>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>

#include "config.hpp"
#include "midi.hpp"

namespace fs = std::filesystem;

static std::vector<std::string_view> split_view(std::string_view str, char delim)
{
	std::string_view view{str};
	std::vector<std::string_view> result;

	for (size_t pos{}, prev{}; pos < view.size() && prev < view.size(); prev = pos + 1) {
		pos = view.find(delim, prev);

		if (pos == view.npos) {
			pos = view.length();
		}

		result.emplace_back(view.substr(prev, pos - prev));
	}

	return result;
}

static const std::unordered_map<std::string, MIDI::Command> command_mapping = {
#define X(name, str) {str, MIDI::Command::name},
	LIST_OF_COMMANDS
#undef X
};

namespace MIDI
{

static int svtoi(std::string_view sv)
{
	int value;
	auto result = std::from_chars(sv.data(), sv.data() + sv.size(), value);

	if (result.ptr != sv.data() + sv.size()) {
		throw std::runtime_error("Conversion error");
	}

	return value;
}


static Message parse_message_str(const std::string &str)
{
	Message message{};
	auto tokens = split_view(str, '/');
	size_t i{};
	uint8_t status{};

	// Get the channel number, if present
	if (!tokens.size() || tokens[0].empty()) {
		throw std::runtime_error("Invalid message string");
	}

	if (isdigit(tokens[i][0])) {
		auto channel = svtoi(tokens[i]);

		if (channel < 1 || channel > 16) {
			throw std::runtime_error("Invalid message string");
		}

		status = channel - 1;
		i++;
	}

	// Get the message type
	if (tokens.size() <= i) {
		throw std::runtime_error("Invalid message string");
	}

	if (tokens[i] == "key") {
		status |= 0x90;
		i++;
	} else if (tokens[i] == "cc") {
		status |= 0xb0;
		i++;
	} else if (tokens[i] == "sysex") {
		status = 0xf0;
		i++;
	} else {
		throw std::runtime_error("Invalid message string");
	}

	message.status = status;

	// Get the key/cc number
	if (tokens.size() <= i) {
		throw std::runtime_error("Invalid message string");
	}

	if (status != 0xf0) {
		auto [ptr, ec] = std::from_chars(tokens[i].data(), tokens[i].data() + tokens[i].size(), message.data);

		if (ec != std::errc{}) {
			throw std::runtime_error("Invalid message string");
		}
	} else {
		// TODO
	}

	return message;
}

static Control parse_control_str(std::string str)
{
	Control control{};
	auto alternatives = split_view(str, ',');
	auto tokens = split_view(alternatives[0], '/');
	size_t i{};

	if (tokens.size() <= i) {
		throw std::runtime_error("Invalid control string");
	}

	if (!tokens[i].empty() && tokens[i].back() == '~') {
		control.toggle = true;
		tokens[i].remove_suffix(1);
	}

	if (!tokens[i].empty() && tokens[i].back() == '+') {
		control.modify = true;
		control.value = 1;
		tokens[i].remove_suffix(1);
	}

	if (!tokens[i].empty() && tokens[i].back() == '-') {
		control.modify = true;
		control.value = -1;
		tokens[i].remove_suffix(1);
	}

	if (!tokens[i].empty() && tokens[i].back() == '*') {
		control.modify = true;
		control.infinite = true;
		tokens[i].remove_suffix(1);
	}

	if (auto it = command_mapping.find(std::string(tokens[i])); it != command_mapping.end()) {
		control.command = it->second;
	} else {
		throw std::runtime_error("Invalid control string");
	}

	if (tokens.size() <= ++i) {
		return control;
	}

	if (tokens[i] == "master") {
		control.master = true;
	} else if (tokens[i] == "top" && tokens.size() > i + 1) {
		control.master = true;
		control.row = -1;
		control.col = svtoi(tokens[++i]) - 1;
	} else if (tokens[i] == "bottom" && tokens.size() > i + 1) {
		control.master = true;
		control.row = 1;
		control.col = svtoi(tokens[++i]) - 1;
	} else if (tokens[i] == "left" && tokens.size() > i + 1) {
		control.master = true;
		control.col = -1;
		control.row = svtoi(tokens[++i]) - 1;
	} else if (tokens[i] == "right" && tokens.size() > i + 1) {
		control.master = true;
		control.col = 1;
		control.row = svtoi(tokens[++i]) - 1;
	} else {
		control.col = svtoi(tokens[i]) - 1;

		if (tokens.size() > i + 1) {
			control.col = svtoi(tokens[++i]);
		}
	}

	return control;
}

void Controller::load(const std::string &hwid)
{
	fs::path filename = "controllers";
	filename /= hwid;
	auto path = config.get_load_path(filename);

	if (hwid.empty() || !fs::exists(path)) {
		path = config.get_load_path("controllers/default");
	}

	auto controller_config = YAML::LoadFile(path);

	info.hwid = hwid;
	info.brand = controller_config["brand"].as<std::string>("Unknown brand");
	info.model = controller_config["model"].as<std::string>("Unknown model");
	info.keys = controller_config["keys"].as<uint8_t>(0);
	info.buttons = controller_config["buttons"].as<uint8_t>(0);
	info.faders = controller_config["faders"].as<uint8_t>(0);
	info.pots = controller_config["pots"].as<uint8_t>(0);
	info.pads = controller_config["pads"].as<uint8_t>(0);
	info.decks = controller_config["decks"].as<uint8_t>(0);
	info.banks = controller_config["banks"].as<uint8_t>(0);
	// TODO: grid

	omni = controller_config["omni"].as<bool>(false);

	std::cerr << hwid << " is a " << controller_config["brand"].as<std::string>("Unknown brand") << " " << controller_config["model"].as<std::string>("Unknown model") << "\n";

	for (auto it = controller_config["mapping"].begin(); it != controller_config["mapping"].end(); ++it) {
		auto message_str = it->first.as<std::string>();
		auto control_str = it->second.as<std::string>();

		Message message;
		Control control;

		try {
			message = parse_message_str(message_str);
		} catch (std::runtime_error &e) {
			std::cerr << "Error parsing \'" << message_str << "\' in " << path << "\n";
			continue;
		}

		try {
			control = parse_control_str(control_str);
		} catch (std::runtime_error &e) {
			std::cerr << "Error parsing \'" << control_str << "\' in " << path << "\n";
			continue;
		}

		mapping[message] = control;
	}
}

}
