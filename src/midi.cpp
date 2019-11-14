/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "midi.hpp"

#include <fmt/format.h>
#include <stdexcept>
#include <unistd.h>

namespace MIDI {

Port::Port(const snd_rawmidi_info_t *info, bool open_in, bool open_out):
	name(snd_rawmidi_info_get_subdevice_name(info))
{
	int card = snd_rawmidi_info_get_card(info);
	int device = snd_rawmidi_info_get_device(info);
	int sub = snd_rawmidi_info_get_subdevice(info);
	auto hw_name = fmt::format("hw:{},{},{}", card, device, sub);

	if (int err = snd_rawmidi_open(open_in ? &in : nullptr, open_out ? &out: nullptr, hw_name.c_str(), SND_RAWMIDI_NONBLOCK); err < 0)
		throw std::runtime_error(snd_strerror(err));
}

Port::~Port() {
	if (in)
		snd_rawmidi_close(in);

	if (out)
		snd_rawmidi_close(out);
}

Manager::Manager(ProgramManager &programs): programs(programs) {
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

			snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
			snd_ctl_rawmidi_info(ctl, info);
			int subs_out = snd_rawmidi_info_get_subdevices_count(info);

			int subs = std::max(subs_in, subs_out);

			for (int sub = 0; sub < subs; sub++) {
				snd_rawmidi_info_set_stream(info, sub < subs_in ? SND_RAWMIDI_STREAM_INPUT : SND_RAWMIDI_STREAM_OUTPUT);
				snd_rawmidi_info_set_subdevice(info, sub);

				if (snd_ctl_rawmidi_info(ctl, info) < 0)
					continue;

				Port &port = ports.emplace_back(info, sub < subs_in, sub < subs_out);

				for(auto &channel: port.channels) {
					channel.program = programs.activate(0);
				}

				if (sub < subs_in) {
					pfds.emplace_back();
					snd_rawmidi_poll_descriptors(ports.back().in, &pfds.back(), 1);
				}
			}
		}

		snd_ctl_close(ctl);
	}

	snd_rawmidi_info_free(info);

	// Add a self-pipe.
	if (pipe(pipe_fds))
		throw std::runtime_error("Could not create pipe");

	pfds.emplace_back();
	auto &pfd = pfds.back();
	pfd.fd = pipe_fds[0];
	pfd.events = POLLIN | POLLERR | POLLHUP;

	thread = std::thread(&Manager::process_events, this);
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
		fprintf(stderr, "Control change for %p", (void *)program);
		program->control_change(data[1], data[2]);
		break;
	case 0xc:
		// program change
		fprintf(stderr, "Program change from %p", (void *)channel.program.get());
		programs.change(channel.program, data[1]);
		fprintf(stderr, " to %p\n", (void *)channel.program.get());
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
		auto err = poll(&pfds[0], pfds.size(), -1);
		uint8_t buf[128];

		if (err < 0)
			throw std::runtime_error(strerror(errno));

		if (err == 0)
			throw std::runtime_error("poll() returned 0");

		for (size_t i = 0; i < pfds.size(); ++i) {
			auto revents = pfds[i].revents;

			if (revents & (POLLERR | POLLHUP))
				throw std::runtime_error(strerror(errno));

			if (revents & POLLIN) {
				if (i + 1 == pfds.size()) {
					return;
				}

				auto len = snd_rawmidi_read(ports[i].in, buf, sizeof buf);

				if (len < 0)
					throw std::runtime_error(snd_strerror(err));

				// TODO: split into individual MIDI commands

				if (len > 0)
					process_midi_command(ports[i], buf, len);
			}
		}
	}
}

}
