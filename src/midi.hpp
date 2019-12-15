/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <alsa/asoundlib.h>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <poll.h>
#include <string>
#include <thread>
#include <vector>

#include "program-manager.hpp"

namespace MIDI {

/**
 * The state for a MIDI channel.
 */
struct Channel {
	std::shared_ptr<Program> program;
};

/**
 * A MIDI port.
 */
class Port {
	friend class Manager;

	snd_rawmidi_t *in{};
	snd_rawmidi_t *out{};

	int card{-1};
	int device{-1};
	int sub{-1};
	std::string name;
	std::string hwid;

	mutable std::mutex last_command_mutex;
	std::vector<uint8_t> last_command;

	Channel channels[16];

	public:
	Port(const snd_rawmidi_info_t *info);
	~Port();

	Port(const Port &other) = delete;
	Port(Port &&other) = delete;
	Port &operator=(const Port &other) = delete;

	void close();
	void open(const snd_rawmidi_info_t *info);
	bool is_match(const snd_rawmidi_info_t *info);

	void panic();

	std::string get_name() const {
		return name;
	}

	bool is_open() const {
		return in;
	}

	void set_last_command(const uint8_t *data, ssize_t len) {
		std::lock_guard lock(last_command_mutex);

		if (len < 0) {
			last_command.clear();
		} else {
			last_command.resize(len);
			std::copy(data, data + len, last_command.begin());
		}
	}

	std::vector<uint8_t> get_last_command() const {
		std::lock_guard lock(last_command_mutex);
		return last_command;
	}

	const Channel &get_channel(uint8_t channel) {
		return channels[channel];
	}
};

/**
 * The manager for all MIDI state.
 */
class Manager {
	Program::Manager &programs;
	std::deque<Port> ports;
	std::vector<struct pollfd> pfds;
	std::thread thread;
	int pipe_fds[2]{-1, -1};
	Port *last_active_port{};

	void process_midi_command(Port &port, const uint8_t *data, ssize_t len);
	void process_events();
	void scan_ports();

	public:
	Manager(Program::Manager &programs);
	~Manager();

	Manager(const Manager &other) = delete;
	Manager(Manager &&other) = delete;
	Manager &operator=(const Manager &other) = delete;

	void start();
	void panic();

	const std::deque<Port> &get_ports() const {
		return ports;
	};

	const Port *get_last_active_port() const {
		return last_active_port;
	}
};

std::string command_to_text(const std::vector<uint8_t> &data);

extern Manager manager;

}
