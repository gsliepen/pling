/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <alsa/asoundlib.h>
#include <deque>
#include <functional>
#include <list>
#include <memory>
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
	std::string get_name() {
		return name;
	}

	bool is_open() {
		return in;
	}
};

/**
 * The manager for all MIDI state.
 */
class Manager {
	ProgramManager &programs;
	std::deque<Port> ports;
	std::vector<struct pollfd> pfds;
	std::thread thread;
	int pipe_fds[2]{-1, -1};

	void process_midi_command(Port &port, const uint8_t *data, ssize_t len);
	void process_events();
	void scan_ports();

	public:
	Manager(ProgramManager &programs);
	~Manager();

	Manager(const Manager &other) = delete;
	Manager(Manager &&other) = delete;
	Manager &operator=(const Manager &other) = delete;

	void panic();
};

extern Manager manager;

}
