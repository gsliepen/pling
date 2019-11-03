/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <alsa/asoundlib.h>
#include <functional>
#include <list>
#include <poll.h>
#include <string>
#include <vector>

namespace MIDI {
	class Port {
		friend class Manager;

		snd_rawmidi_t *in{};
		snd_rawmidi_t *out{};

		public:
		Port(const snd_rawmidi_info_t *info, bool open_in, bool open_out);
		~Port();

		Port(const Port &other) = delete;
		Port(Port &&other) {
			in = other.in;
			out = other.out;
			other.in = nullptr;
			other.out = nullptr;
		}
		Port &operator=(const Port &other) = delete;

		const std::string name;
	};

	class Manager {
		std::vector<Port> ports;
		std::vector<struct pollfd> pfds;

		public:
		Manager();
		~Manager();

		Manager(const Manager &other) = delete;
		Manager(Manager &&other) = delete;
		Manager &operator=(const Manager &other) = delete;

		void process_events(std::function<void(Port &port, uint8_t *data, ssize_t len)> callback);
	};
}
