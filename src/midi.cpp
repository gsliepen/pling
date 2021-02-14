/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "midi.hpp"

#include <algorithm>
#include <iostream>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

#include "controller.hpp"
#include "state.hpp"
#include "utils.hpp"

namespace MIDI
{

Port::Port(snd_seq_t *seq, const snd_seq_port_info_t *info)
{
	open(seq, info);
}

Port::~Port()
{
	close();
}

void Port::open(snd_seq_t *seq, const snd_seq_port_info_t *info)
{
	client = snd_seq_port_info_get_client(info);
	port = snd_seq_port_info_get_port(info);

	snd_seq_client_info_t *cinfo;
	snd_seq_client_info_malloc(&cinfo);
	snd_seq_get_any_client_info(seq, client, cinfo);
	int card = snd_seq_client_info_get_card(cinfo);
	snd_seq_client_info_free(cinfo);

	name = snd_seq_port_info_get_name(info);

	auto filename = fmt::format("/proc/asound/card{}/usbid", card);
	std::ifstream usbid_file(filename);

	std::string line;

	if (std::getline(usbid_file, line)) {
		hwid = line;
	} else {
		hwid = name;
	}

	// Match it to a MIDI controller definition
	controller.load(hwid);
}

bool Port::is_match(snd_seq_t *seq, const snd_seq_port_info_t *info)
{
	if (name != snd_seq_port_info_get_name(info)) {
		return false;
	}

	auto other_client = snd_seq_port_info_get_client(info);

	snd_seq_client_info_t *cinfo;
	snd_seq_client_info_malloc(&cinfo);
	snd_seq_get_any_client_info(seq, other_client, cinfo);
	int card = snd_seq_client_info_get_card(cinfo);
	snd_seq_client_info_malloc(&cinfo);

	auto filename = fmt::format("/proc/asound/card{}/usbid", card);
	std::ifstream usbid_file(filename);

	std::string line;

	if (std::getline(usbid_file, line)) {
		if (line != hwid) {
			return false;
		}
	}

	return true;
}

void Port::close()
{
	panic();

	client = -1;
	port = -1;
}

void Port::panic()
{
	for (auto &channel : channels) {
		channel.program->release_all();
	}
}

Manager::Manager(Program::Manager &programs): programs(programs)
{
	snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0);
	snd_seq_nonblock(seq, true);
	snd_seq_set_client_name(seq, "pling");
	snd_seq_create_simple_port(seq, "pling",
	                           SND_SEQ_PORT_CAP_WRITE |
	                           SND_SEQ_PORT_CAP_SUBS_WRITE,
	                           SND_SEQ_PORT_TYPE_SOFTWARE |
	                           SND_SEQ_PORT_TYPE_SYNTHESIZER |
	                           SND_SEQ_PORT_TYPE_APPLICATION);

	// Add a self-pipe.
	if (pipe(pipe_fds)) {
		throw std::runtime_error("Could not create pipe");
	}

	auto &pfd = pfds.emplace_back();
	pfd.fd = pipe_fds[0];
	pfd.events = POLLIN | POLLERR | POLLHUP;
}

Manager::~Manager()
{
	write(pipe_fds[1], "Q", 1);

	thread.join();

	close(pipe_fds[0]);
	close(pipe_fds[1]);

	snd_seq_delete_port(seq, 0);
	snd_seq_close(seq);
}

void Manager::start()
{
	snd_seq_connect_from(seq, 0, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);
	scan_ports();
	update_pfds();

	thread = std::thread(&Manager::process_events, this);
}

void Manager::add_port(const snd_seq_port_info_t *pinfo)
{
	if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) != (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) {
		return;
	}

	int client = snd_seq_port_info_get_client(pinfo);
	int portnr = snd_seq_port_info_get_port(pinfo);

	for (auto &port : ports) {
		if (port.is_match(seq, pinfo)) {
			if (port.is_open()) {
				std::cerr << "Adding port found existing open port that matches!\n";
			} else {
				port.open(seq, pinfo);
				auto result = snd_seq_connect_from(seq, 0, client, portnr);

				if (result < 0) {
					std::cerr << "Could not connect to port!\n";
					return;
				}
			}

			return;
		}
	}

	auto result = snd_seq_connect_from(seq, 0, client, portnr);

	if (result < 0) {
		std::cerr << "Could not connect to port!\n";
		return;
	}

	bool is_first_port = ports.empty();
	Port &port = ports.emplace_back(seq, pinfo);

	if (is_first_port) {
		state.set_active_channel(port, 0);
		last_active_port = &port;
	}

	for (auto &channel : port.channels) {
		programs.change(channel.program, 0);
	}
}

void Manager::update_pfds()
{
	auto npfds = snd_seq_poll_descriptors_count(seq, POLLIN);
	pfds.resize(npfds + 1);
	snd_seq_poll_descriptors(seq, pfds.data() + 1, npfds, POLLIN);
}

void Manager::scan_ports()
{
	snd_seq_client_info_t *cinfo;
	snd_seq_port_info_t *pinfo;
	snd_seq_client_info_malloc(&cinfo);
	snd_seq_port_info_malloc(&pinfo);

	snd_seq_client_info_set_client(cinfo, -1);

	while (snd_seq_query_next_client(seq, cinfo) >= 0) {
		int client = snd_seq_client_info_get_client(cinfo);

		if (client == SND_SEQ_CLIENT_SYSTEM) {
			continue;
		}

		snd_seq_port_info_set_client(pinfo, client);
		snd_seq_port_info_set_port(pinfo, -1);
		int card = snd_seq_client_info_get_card(cinfo);

		if (card == -1) {
			continue;
		}

		while (snd_seq_query_next_port(seq, pinfo) >= 0) {
			add_port(pinfo);
		}
	}

	snd_seq_port_info_malloc(&pinfo);
	snd_seq_client_info_malloc(&cinfo);
}

void Manager::process_system_event(const snd_seq_event_t &event)
{
	switch (event.type) {
	case SND_SEQ_EVENT_PORT_START: {
		snd_seq_port_info_t *pinfo;
		snd_seq_port_info_malloc(&pinfo);
		snd_seq_get_any_port_info(seq, event.data.addr.client, event.data.addr.port, pinfo);
		add_port(pinfo);
		snd_seq_port_info_free(pinfo);
		update_pfds();
	}
	break;

	case SND_SEQ_EVENT_PORT_EXIT: {
		auto port_it = std::find_if(ports.begin(), ports.end(), [event](const auto & port) {
			return port.client == event.data.addr.client && port.port == event.data.addr.port;
		});

		if (port_it != ports.end()) {
			port_it->close();
			update_pfds();
		}
	}
	break;


	default:
		break;

	}

}

void Manager::process_seq_event(const snd_seq_event_t &event)
{
	auto port_it = std::find_if(ports.begin(), ports.end(), [event](const auto & port) {
		return port.client == event.source.client && port.port == event.source.port;
	});

	if (port_it == ports.end()) {
		if (event.source.client == SND_SEQ_CLIENT_SYSTEM) {
			process_system_event(event);
			return;
		}

		fmt::print(std::cerr, "Unknown source {}:{}\n", event.source.client, event.source.port);
		return;
	}

	auto &port = *port_it;

	Control control = port.controller.map(event);

	if (control.command != Command::PASS) {
		uint8_t value;

		switch (event.type) {
		case SND_SEQ_EVENT_NOTEON:
		case SND_SEQ_EVENT_NOTEOFF:
			value = event.data.note.velocity;
			break;

		case SND_SEQ_EVENT_CONTROLLER:
			value = event.data.control.value;
			break;

		default:
			value = 0;
			break;
		}

		state.process_control(control, port, value);
		return;
	}

	auto &channel = port.channels[event.data.control.channel & 0xf];
	auto program = channel.program;

	switch (event.type) {
	case SND_SEQ_EVENT_NOTEON:
		if (event.data.note.velocity) {
			programs.activate(program);
			program->note_on(event.data.note.note, event.data.note.velocity);
			state.note_on(event.data.note.note, event.data.note.velocity);
		} else {
			program->note_off(event.data.note.note, event.data.note.velocity);
			state.note_off(event.data.note.note);
		}

		break;

	case SND_SEQ_EVENT_NOTEOFF:
		program->note_off(event.data.note.note, event.data.note.velocity);
		state.note_off(event.data.note.note);
		break;

	case SND_SEQ_EVENT_KEYPRESS: // Polyphonic pressure
		program->poly_pressure(event.data.note.note, event.data.note.velocity);
		break;

	case SND_SEQ_EVENT_CONTROLLER:
		switch (event.data.control.param) {
		case MIDI_CTL_MSB_MODWHEEL:
			program->modulation(event.data.control.value);
			break;

		case MIDI_CTL_SUSTAIN:
			program->sustain(event.data.control.value & 64);
			break;

		default:
			break;
		}

		break;

	case SND_SEQ_EVENT_PGMCHANGE:
		state.set_active_channel(port, event.data.control.channel);
		programs.change(channel.program, event.data.control.value);
		state.set_active_program(channel.program);
		break;

	case SND_SEQ_EVENT_CHANPRESS:
		program->channel_pressure(event.data.control.value);
		break;

	case SND_SEQ_EVENT_PITCHBEND:
		program->pitch_bend(event.data.control.value);
		state.set_bend(event.data.control.value);
		break;

	default:
		break;
	}
}

void Manager::process_events()
{
	while (true) {
		auto result = poll(pfds.data(), pfds.size(), -1);

		if (result < 0) {
			throw std::runtime_error(strerror(errno));
		}

		if (result == 0) {
			continue;
		}

		// First fd is the self-pipe, if anything happens on it we quit this thread.
		if (pfds[0].revents) {
			break;
		}

		// Else, assume we got a sequencer event
		while (true) {
			snd_seq_event_t *event;
			auto left = snd_seq_event_input(seq, &event);

			if (event) {
				process_seq_event(*event);
			}

			if (left <= 0) {
				break;
			}
		}
	}
}

void Manager::panic()
{
	for (auto &port : ports) {
		port.panic();
	}

	state.release_all();
}

std::string event_to_text(const snd_seq_event_t &event)
{
	auto &data = event.data;

	switch (event.type) {
	case SND_SEQ_EVENT_NOTEOFF:
		return fmt::format("channel {} note-off key {} vel {}", data.note.channel, data.note.note, data.note.velocity);

	case SND_SEQ_EVENT_NOTEON:
		return fmt::format("channel {} note-on key {} vel {}", data.note.channel, data.note.note, data.note.velocity);

	case SND_SEQ_EVENT_KEYPRESS:
		return fmt::format("channel {} polyphonic-pressure key {} value {}", data.note.channel, data.note.note, data.note.velocity);

	case SND_SEQ_EVENT_CONTROLLER:
		return fmt::format("channel {} control-change {} value {}", data.control.channel, data.control.param, data.control.value);

	case SND_SEQ_EVENT_PGMCHANGE:
		return fmt::format("channel {} program-change {}", data.control.channel, data.control.value);

	case SND_SEQ_EVENT_CHANPRESS:
		return fmt::format("channel {} channel-pressure {}", data.control.channel, data.control.value);

	case SND_SEQ_EVENT_PITCHBEND:
		return fmt::format("channel {} pitch-bend value {}", data.control.channel, data.control.value);

	case SND_SEQ_EVENT_SYSEX:
		return fmt::format("sysex");

	case SND_SEQ_EVENT_QFRAME:
		return fmt::format("time-code-quarter-frame value {}", data.control.value);

	case SND_SEQ_EVENT_SONGPOS:
		return fmt::format("song-position-pointer {}", data.control.value);

	case SND_SEQ_EVENT_SONGSEL:
		return fmt::format("song-select {}", data.control.value);

	case SND_SEQ_EVENT_TUNE_REQUEST:
		return "tune-request";
		break;

	default:
		return "unknown";
	}
}

}
