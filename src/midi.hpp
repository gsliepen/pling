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

#include "controller.hpp"
#include "program-manager.hpp"

namespace MIDI
{

/**
 * The state for a MIDI channel.
 */
struct Channel {
	std::shared_ptr<Program> program;
};

/**
 * A MIDI port.
 */
class Port
{
	friend class Manager;

	int client{-1};
	int port{-1};

	std::string name;
	std::string hwid;

	mutable std::mutex last_event_mutex;
	snd_seq_event_t last_event;

	Controller controller;
	Channel channels[16];

public:
	Port(snd_seq_t *seq, const snd_seq_port_info_t *info);
	~Port();

	Port(const Port &other) = delete;
	Port(Port &&other) = delete;
	Port &operator=(const Port &other) = delete;

	void close();
	void open(snd_seq_t *seq, const snd_seq_port_info_t *info);
	bool is_match(snd_seq_t *seq, const snd_seq_port_info_t *info);

	void panic();

	std::string get_name() const
	{
		return name;
	}

	const std::string &get_hwid() const
	{
		return hwid;
	}

	bool is_open() const
	{
		return client != -1;
	}

	void set_last_event(const snd_seq_event_t &event)
	{
		std::lock_guard lock(last_event_mutex);
		last_event = event;
	}

	snd_seq_event_t get_last_event() const
	{
		std::lock_guard lock(last_event_mutex);
		return last_event;
	}

	const Channel &get_channel(uint8_t channel)
	{
		return channels[channel];
	}
};

/**
 * The manager for all MIDI state.
 */
class Manager
{
	Program::Manager &programs;
	std::deque<Port> ports;
	std::vector<struct pollfd> pfds;
	std::thread thread;
	int pipe_fds[2] {-1, -1};
	Port *last_active_port{};
	snd_seq_t *seq{};
	snd_midi_event_t *event_parser{};

	void update_pfds();
	void add_port(const snd_seq_port_info_t *pinfo);
	void process_midi_command(Port &port, const uint8_t *data, ssize_t len);
	void process_system_event(const snd_seq_event_t &event);
	void process_seq_event(const snd_seq_event_t &event);
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

	std::deque<Port> &get_ports()
	{
		return ports;
	};

	const Port *get_last_active_port() const
	{
		return last_active_port;
	}

	void change(Port *port, uint8_t channel, uint8_t MIDI_program, uint8_t bank_lsb = 0, uint8_t bank_msb = 0)
	{
		programs.change(port->channels[channel].program, MIDI_program, bank_lsb, bank_msb);
	}
};

std::string event_to_text(const snd_seq_event_t &event);

extern Manager manager;

}
