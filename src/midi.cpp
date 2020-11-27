/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "midi.hpp"

#include <iostream>
#include <fmt/format.h>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

#include "controller.hpp"
#include "state.hpp"
#include "utils.hpp"

namespace MIDI
{

Port::Port(const snd_rawmidi_info_t *info)
{
	open(info);
}

Port::~Port()
{
	close();
}

void Port::open(const snd_rawmidi_info_t *info)
{
	card = snd_rawmidi_info_get_card(info);
	device = snd_rawmidi_info_get_device(info);
	sub = snd_rawmidi_info_get_subdevice(info);

	auto hw_name = fmt::format("hw:{},{},{}", card, device, sub);

	name = snd_rawmidi_info_get_subdevice_name(info);

	auto filename = fmt::format("/proc/asound/card{}/usbid", card);
	std::ifstream usbid_file(filename);

	std::string line;

	if (std::getline(usbid_file, line)) {
		hwid = line;
	}

	if (int err = snd_rawmidi_open(&in, nullptr, hw_name.c_str(), SND_RAWMIDI_NONBLOCK); err < 0) {
		return;
	}

	snd_rawmidi_drain(in);

	// Try to open the output fd, but it's fine if there is none.
	snd_rawmidi_open(nullptr, &out, hw_name.c_str(), SND_RAWMIDI_NONBLOCK);

	// Match it to a MIDI controller definition
	controller.load(hwid);
}

bool Port::is_match(const snd_rawmidi_info_t *info)
{
	int card = snd_rawmidi_info_get_card(info);

	if (name != snd_rawmidi_info_get_subdevice_name(info)) {
		return false;
	}

	auto filename = fmt::format("/proc/asound/card{}/usbid", card);
	std::ifstream usbid_file(filename);

	std::string line;

	if (std::getline(usbid_file, line)) {
		if (line == hwid) {
			return true;
		}
	}

	return false;
}

void Port::close()
{
	panic();

	snd_rawmidi_close(in);

	if (out) {
		snd_rawmidi_close(out);
	}

	in = {};
	out = {};
	card = -1;
	device = -1;
	sub = -1;
}

void Port::panic()
{
	for (auto &channel : channels) {
		channel.program->release_all();
	}
}

Manager::Manager(Program::Manager &programs): programs(programs)
{
	// Add a self-pipe.
	if (pipe(pipe_fds)) {
		throw std::runtime_error("Could not create pipe");
	}

	pfds.emplace_back();
	auto &pfd = pfds.back();
	pfd.fd = pipe_fds[0];
	pfd.events = POLLIN | POLLERR | POLLHUP;
}

void Manager::start()
{
	scan_ports();

	thread = std::thread(&Manager::process_events, this);
}

void Manager::scan_ports()
{
	snd_rawmidi_info_t *info;

	if (int err = snd_rawmidi_info_malloc(&info); err < 0) {
		throw std::runtime_error(snd_strerror(err));
	}

	for (int card = -1; snd_card_next(&card) >= 0 && card >= 0;) {
		snd_ctl_t *ctl;
		auto name = fmt::format("hw:{}", card);

		if (snd_ctl_open(&ctl, name.c_str(), 0) < 0) {
			continue;
		}

		for (int device = -1; snd_ctl_rawmidi_next_device(ctl, &device) >= 0 && device >= 0;) {
			snd_rawmidi_info_set_device(info, device);

			snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
			snd_ctl_rawmidi_info(ctl, info);
			int subs_in = snd_rawmidi_info_get_subdevices_count(info);

			// Ignore MIDI devices that don't send data to us
			if (!subs_in) {
				continue;
			}

			for (int sub = 0; sub < subs_in; sub++) {
				snd_rawmidi_info_set_subdevice(info, sub);

				if (snd_ctl_rawmidi_info(ctl, info) < 0) {
					continue;
				}

				bool found = false;

				for (size_t i = 0; i < ports.size(); ++i) {
					auto &port = ports[i];

					if (port.card == card && port.device == device && port.sub == sub && port.is_open()) {
						// We already have this exact port open, skip it.
						found = true;
						break;
					}

					if (port.is_open() || !port.is_match(info)) {
						continue;
					}

					/* We found a matching, unused port.
					 * Assume it's the same physical device, and just reopen it.
					 */
					port.open(info);

					if (port.is_open()) {
						snd_rawmidi_poll_descriptors(port.in, &pfds[i + 1], 1);
					}

					found = true;
					break;
				}

				if (found) {
					continue;
				}

				bool is_first_port = ports.empty();

				Port &port = ports.emplace_back(info);

				if (is_first_port) {
					state.set_active_channel(port, 0);
					last_active_port = &port;
				}

				for (auto &channel : port.channels) {
					programs.change(channel.program, 0);
				}

				pfds.emplace_back();

				if (ports.back().is_open()) {
					snd_rawmidi_poll_descriptors(ports.back().in, &pfds.back(), 1);
				}
			}
		}

		snd_ctl_close(ctl);
	}

	snd_rawmidi_info_free(info);
}

Manager::~Manager()
{
	write(pipe_fds[1], "Q", 1);

	thread.join();

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

void Manager::process_midi_command(Port &port, const uint8_t *data, ssize_t len)
{
	// In learning mode we should not react to any MIDI command, but rather remember the last command.
	if (state.get_learn_midi()) {
		port.set_last_command(data, len);
		return;
	}

	// Try to map the MIDI message to a control
	uint8_t type = data[0] >> 4;
	uint8_t chan = data[0] & 0xf;

	Channel &channel = port.channels[chan];
	Control control = port.controller.map({data[0], data[1]});

	if (control.command != Command::PASS) {
		state.process_control(control, port, data, len);
		return;
	}

	// Process non-control messages
	auto program = channel.program;

	if (!program) {
		return;
	}

	switch (type) {
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x3:
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		// This should never happen in the first byte.
		break;

	// Channel voice messages
	// ----------------------
	case 0x8: // Note off
		program->note_off(data[1], data[2]);
		state.note_off(data[1]);
		break;

	case 0x9: // Note on
		if (data[2]) {
			programs.activate(program);
			program->note_on(data[1], data[2]);
			state.note_on(data[1], data[2]);
		} else {
			program->note_off(data[1], data[2]);
			state.note_off(data[1]);
		}

		break;

	case 0xa: // Polyphonic pressure
		program->poly_pressure(data[1], data[2]);
		break;

	case 0xb: // Control change

		// Only handle controls defined in GM
		switch (data[1]) {
		case 1:
			program->modulation(data[2]);
			break;

		case 64:
			program->sustain(data[2] & 64);
			break;

		default:
			break;
		}

		break;

	case 0xc: // Program change
		state.set_active_channel(port, chan);
		programs.change(channel.program, data[1]);
		state.set_active_program(channel.program);
		break;

	case 0xd: // Channel pressure
		program->channel_pressure(data[1]);
		break;

	case 0xe: { // Pitch bend
		int16_t value = (data[1] | (data[2] << 7)) - 8192;
		program->pitch_bend(value);
		state.set_bend(value);
		break;
	}

	case 0xf: // Non-channel messages
		switch (data[0] & 0x0f) {
		// System common messages
		// ----------------------
		case 0x0: // system exclusive
			break;

		case 0x1: // time code
			break;

		case 0x2: // song position pointer
			break;

		case 0x3: // song select
			break;

		case 0x4: // (reserved)
			break;

		case 0x5: // (reserved)
			break;

		case 0x6: // tune request
			break;

		case 0x7: // end of system exclusive
			break;

		// System real-time messages
		// -------------------------
		case 0x8: // timing clock
			break;

		case 0x9: // (reserved)
			break;

		case 0xa: // start
			break;

		case 0xb: // continue
			break;

		case 0xc: // stop
			break;

		case 0xd: // (undefined)
			break;

		case 0xe: // active sensing
			break;

		case 0xf: // reset
			break;
		}

		// TODO: handle partial SysEx messages?
		break;
	}
}

void Manager::process_events()
{
	while (true) {
		auto result = poll(&pfds[0], pfds.size(), 1000);
		uint8_t buf[128];

		if (result < 0) {
			throw std::runtime_error(strerror(errno));
		}

		if (result == 0) {
			scan_ports();
			continue;
		}

		// First fd is the self-pipe, if anything happens on it we quit this thread.
		if (pfds[0].revents) {
			break;
		}

		for (size_t i = 1; i < pfds.size(); ++i) {
			auto revents = pfds[i].revents;
			auto &port = ports[i - 1];

			if (revents & (POLLERR | POLLHUP)) {
				port.close();
				pfds[i].fd = -1;
			}

			if (revents & POLLIN) {
				auto len = snd_rawmidi_read(port.in, buf, sizeof buf);

				if (len < 0) {
					port.close();
					pfds[i].fd = -1;
				}

				last_active_port = &port;

				decltype(len) start = 0;
				decltype(len) end;

				// TODO: filter out real-time messages
				for (end = 1; end < len; end++) {
					if (buf[end] & 0x80 && (buf[end] != 0xf7 || buf[start] != 0xf0)) {
						process_midi_command(port, buf + start, end - start);
						start = end;
					}
				}

				if (start < len) {
					process_midi_command(port, buf + start, len - start);
				}
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

std::string command_to_text(const std::vector<uint8_t> &data)
{
	if (data.empty())
		return {};

	uint8_t type = data[0] >> 4;

	uint8_t chan = data[0] & 0xf;

	switch (type) {
	case 0x0:
	case 0x1:
	case 0x2:
	case 0x3:
	case 0x4:
	case 0x5:
	case 0x6:
	case 0x7:
		// This should never happen in the first byte.
		break;

	// Channel voice messages
	// ----------------------
	case 0x8:
		if (data.size() != 3) {
			break;
		}

		return fmt::format("channel {} note-off key {} vel {}", chan + 1, data[1], data[2]);

	case 0x9:
		if (data.size() != 3) {
			break;
		}

		return fmt::format("channel {} note-on key {} vel {}", chan + 1, data[1], data[2]);

	case 0xa:
		if (data.size() != 3) {
			break;
		}

		return fmt::format("channel {} polyphonic-pressure key {} value {}", chan + 1, data[1], data[2]);

	case 0xb:
		if (data.size() != 3) {
			break;
		}

		return fmt::format("channel {} control-change {} value {}", chan + 1, data[1], data[2]);

	case 0xc:
		if (data.size() != 2) {
			break;
		}

		return fmt::format("channel {} program-change {}", chan + 1, data[1]);

	case 0xd:
		if (data.size() != 2) {
			break;
		}

		return fmt::format("channel {} channel-pressure value {}", chan + 1, data[1]);

	case 0xe: {
		if (data.size() != 3) {
			break;
		}

		int16_t value = (data[1] | (data[2] << 7)) - 8192;
		return fmt::format("channel {} pitch-bend value {}", chan + 1, value);
	}

	case 0xf:
		switch (data[0] & 0x0f) {
		// System common messages
		// ----------------------
		case 0x0: {
			std::string text = "sysex";

			for (size_t i = 1; i < data.size(); ++i) {
				if (data[i] == 0xf7) {
					break;
				}

				text.push_back(' ');
				text.push_back("0123456789ABCDEF"[data[i] >> 4]);
				text.push_back("0123456789ABCDEF"[data[i] & 0xf]);
			}

			return text;
		}

		case 0x1:
			if (data.size() != 2) {
				break;
			}

			return fmt::format("time-code-quarter-frame type {} value {}", data[1] >> 4, data[1] && 0xf);

		case 0x2: {
			if (data.size() != 3) {
				break;
			}

			uint16_t value = (data[1] | (data[2] << 7));
			return fmt::format("song-position-pointer {}", value);
		}

		case 0x3:
			if (data.size() != 2) {
				break;
			}

			return fmt::format("song-select {}", data[1]);

		case 0x4: // (reserved)
			break;

		case 0x5: // (reserved)
			break;

		case 0x6:
			if (data.size() != 1) {
				break;
			}

			return "tune-request";
			break;

		case 0x7:
			if (data.size() != 1) {
				break;
			}

			return "end-of-exclusive";
			break;

		// System real-time messages
		// -------------------------
		case 0x8:
			return "timing-clock";

		case 0x9: // (reserved)
			break;

		case 0xa:
			return "start";

		case 0xb:
			return "continue";

		case 0xc:
			return "stop";

		case 0xd: // (undefined)
			break;

		case 0xe:
			return "active-sensing";

		case 0xf:
			return "reset";
		}

		break;
	}

	std::string text = "unknown";

	for (size_t i = 1; i < data.size() - 1; ++i) {
		text.push_back(' ');
		text.push_back("0123456789ABCDEF"[data[i] >> 4]);
		text.push_back("0123456789ABCDEF"[data[i] & 0xf]);
	}

	return text;
}

}
