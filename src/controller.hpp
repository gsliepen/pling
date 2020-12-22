/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <string>
#include <variant>
#include <unordered_map>
#include <vector>

#include "utils.hpp"

namespace MIDI
{

struct Message {
	uint8_t status;
	uint8_t data;

	bool operator==(const Message &rhs) const noexcept
	{
		return status == rhs.status && data == rhs.data;
	}
};

}

template<> struct std::hash<MIDI::Message> {
	std::size_t operator()(const MIDI::Message &message) const noexcept
	{
		uint16_t value = message.status;
		value |= message.data << 8;
		return std::hash<uint16_t>()(value);
	}
};

namespace MIDI
{

#define LIST_OF_COMMANDS\
	/* Generic controls */\
	X(IGNORE, "ignore")\
	X(POT, "pot")\
	X(FADER, "fader")\
	X(BUTTON, "button")\
	X(PAD, "pad")\
	X(GRID, "grid")\
	/* Transport control */\
	X(LOOP, "loop")\
	X(REWIND, "rewind")\
	X(FORWARD, "forward")\
	X(STOP, "stop")\
	X(PLAY, "play")\
	X(RECORD, "record")\
	/* Other actions */\
	X(HOME, "home")\
	X(SET_LEFT, "set_left")\
	X(SET_RIGHT, "set_right")\
	X(UNDO, "undo")\
	X(CLICK, "click")\
	X(MODE, "mode")\
	X(MIXER, "mixer")\
	X(INSTRUMENT, "instrument")\
	X(PRESET, "preset")\
	X(BANK, "bank")\
	X(CLIPS, "clips")\
	X(SCENES, "scenes")\
	X(PAGES, "pages")\
	X(SHIFT, "shift")\
	X(TRACK, "track")\
	X(PATTERN, "pattern")\
	X(TEMPO, "tempo")\
	X(CROSSFADE, "crossfade")

enum class Command : uint8_t {
	PASS = 0,
#define X(name, str) name,
	LIST_OF_COMMANDS
#undef X
};

struct Control {
	Command command;

	union {
		int8_t row;
		int8_t value;
	};
	int8_t col;

	bool toggle: 1;
	bool infinite: 1;
	bool modify: 1;
	bool master: 1;
};

struct Controller {
	bool omni: 1;          /// Controller is always in omni mode

	std::unordered_map<Message, Control> mapping;

	struct Info {
		std::string hwid;       /// Hardware identifier (USBID)
		std::string brand;      /// Manufacturer name
		std::string model;      /// Model name

		uint8_t keys;           /// Number of physical keys
		uint8_t buttons;        /// Number of generic buttons that send CC codes
		uint8_t faders;         /// Number of linear faders that send CC codes
		uint8_t pots;          /// Number of rotary pots that send CC codes
		uint8_t pads;           /// Number of velocity sensitive pads
		uint8_t decks;          /// Number of DJ decks
		uint8_t banks;          /// Number of visibly selectable banks

		struct Grid {
			uint8_t x;      /// Grid size horizontally, excluding edge buttons
			uint8_t y;      /// Grid size vertically, excluding edge buttons
			bool top;       /// top edge buttons present
			bool left;      /// left edge buttons present
			bool right;     /// right edge buttons present
			bool bottom;    /// bottom edge buttons present
		} grid;                 /// Grid configuration

		struct Features {
			bool program_change: 1; /// Controller can send program change messages
			bool bank_select: 1;   /// Controller can send bank select messages
			bool channel: 1;       /// Controller can switch channel for note on/off messages
			bool pitch_bend: 1;    /// Controller has pitch bend control
			bool modulation: 1;    /// Controller has modulation control
			bool aftertouch: 1;    /// Controller has aftertouch control
			bool sustain: 1;       /// Controller has a sustain pedal control
			bool transport: 1;     /// Controller has transport buttons
			bool cc_mode: 1;       /// Controller can switch keys/pads between note on/off and CC messages
		} features;
	} info;

	void load(const std::string &hwid);

	Control map(Message msg)
	{
		if (auto it = mapping.find(msg); it != mapping.end()) {
			return it->second;
		} else {
			return {};
		}
	}
};

}
