/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "midi.hpp"

#include <fmt/format.h>
#include <stdexcept>

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

	Manager::Manager() {
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

					ports.emplace_back(info, sub < subs_in, sub < subs_out);

					if (sub < subs_in) {
						pfds.emplace_back();
						snd_rawmidi_poll_descriptors(ports.back().in, &pfds.back(), 1);
					}
				}
			}

			snd_ctl_close(ctl);
		}

		snd_rawmidi_info_free(info);
	}

	Manager::~Manager() {
	}

	void Manager::process_events(std::function<void(Port &port, uint8_t *data, ssize_t len)> callback) {
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
				auto len = snd_rawmidi_read(ports[i].in, buf, sizeof buf);

				if (len < 0)
					throw std::runtime_error(snd_strerror(err));

				if (len > 0)
					callback(ports[i], buf, len);
			}
		}

	}
}
