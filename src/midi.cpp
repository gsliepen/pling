/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "midi.hpp"

#include <fmt/format.h>
#include <fstream>
#include <stdexcept>
#include <unistd.h>

namespace MIDI {

Port::Port(const snd_rawmidi_info_t *info) {
	open(info);
}

Port::~Port() {
	close();
}

void Port::open(const snd_rawmidi_info_t *info) {
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
}

bool Port::is_match(const snd_rawmidi_info_t *info) {
	int card = snd_rawmidi_info_get_card(info);

	if (name != snd_rawmidi_info_get_subdevice_name(info))
		return false;

	auto filename = fmt::format("/proc/asound/card{}/usbid", card);
	std::ifstream usbid_file(filename);

	std::string line;
	if (std::getline(usbid_file, line)) {
		if (line == hwid)
			return true;
	}

	return false;
}

void Port::close() {
	snd_rawmidi_close(in);

	if (out)
		snd_rawmidi_close(out);

	in = {};
	out = {};
	card = -1;
	device = -1;
	sub = -1;
}

Manager::Manager(ProgramManager &programs): programs(programs) {
	// Add a self-pipe.
	if (pipe(pipe_fds))
		throw std::runtime_error("Could not create pipe");

	pfds.emplace_back();
	auto &pfd = pfds.back();
	pfd.fd = pipe_fds[0];
	pfd.events = POLLIN | POLLERR | POLLHUP;

	scan_ports();

	thread = std::thread(&Manager::process_events, this);
}

void Manager::scan_ports() {
	snd_rawmidi_info_t *info;

	if (int err = snd_rawmidi_info_malloc(&info); err < 0)
		throw std::runtime_error(snd_strerror(err));

	for (int card = -1; snd_card_next(&card) >= 0 && card >= 0;) {
		snd_ctl_t *ctl;
		auto name = fmt::format("hw:{}", card);

		if (snd_ctl_open(&ctl, name.c_str(), 0) < 0)
			continue;

		for (int device = -1; snd_ctl_rawmidi_next_device(ctl, &device) >= 0 && device >= 0;) {
			snd_rawmidi_info_set_device(info, device);

			snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
			snd_ctl_rawmidi_info(ctl, info);
			int subs_in = snd_rawmidi_info_get_subdevices_count(info);

			// Ignore MIDI devices that don't send data to us
			if (!subs_in)
				continue;

			for (int sub = 0; sub < subs_in; sub++) {
				snd_rawmidi_info_set_subdevice(info, sub);

				if (snd_ctl_rawmidi_info(ctl, info) < 0)
					continue;

				bool found = false;

				for (size_t i = 0; i < ports.size(); ++i) {
					auto &port = ports[i];

					if (port.card == card && port.device == device && port.sub == sub && port.is_open()) {
						// We already have this exact port open, skip it.
						found = true;
						break;
					}

					if (port.is_open() || !port.is_match(info))
						continue;

					/* We found a matching, unused port.
					 * Assume it's the same physical device, and just reopen it.
					 */
					port.open(info);

					if(port.is_open())
						snd_rawmidi_poll_descriptors(port.in, &pfds[i + 1], 1);

					found = true;
					break;
				}

				if (found)
					continue;

				Port &port = ports.emplace_back(info);

				for(auto &channel: port.channels) {
					channel.program = programs.activate(0);
				}

				pfds.emplace_back();

				if(ports.back().is_open())
					snd_rawmidi_poll_descriptors(ports.back().in, &pfds.back(), 1);
			}
		}

		snd_ctl_close(ctl);
	}

	snd_rawmidi_info_free(info);
}

Manager::~Manager() {
	write(pipe_fds[1], "Q", 1);

	thread.join();

	close(pipe_fds[0]);
	close(pipe_fds[1]);
}

void Manager::process_midi_command(Port &port, const uint8_t *data, ssize_t len) {
	uint8_t type = data[0] >> 4;
	uint8_t chan = data[0] & 0xf;

	Channel &channel = port.channels[chan];
	Program *program = channel.program.get();
	assert(program);

	switch(type) {
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
		program->note_off(data[1], data[2]);
		break;
	case 0x9:
		if (data[2]) {
			program->note_on(data[1], data[2]);
		} else {
			program->note_off(data[1], data[2]);
		}
		break;
	case 0xa:
		program->poly_pressure(data[1], data[2]);
		break;
	case 0xb:
		program->control_change(data[1], data[2]);
		break;
	case 0xc:
		// program change
		programs.change(channel.program, data[1]);
		break;
	case 0xd:
		program->channel_pressure(data[1]);
		break;
	case 0xe:
		program->pitch_bend((data[1] | (data[2] << 7)) - 8192);
		break;
	case 0xf:
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

void Manager::process_events() {
	while (true) {
		auto result = poll(&pfds[0], pfds.size(), 1000);
		uint8_t buf[128];

		if (result < 0)
			throw std::runtime_error(strerror(errno));

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

				// TODO: split into individual MIDI commands

				if (len > 0)
					process_midi_command(port, buf, len);
			}
		}
	}
}

}
