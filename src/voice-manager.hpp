/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>

#include "pling.hpp"

/**
 * A manager for a fixed number of polyphonic voices.
 */
template <typename Voice, uint32_t N>
class VoiceManager {
	uint32_t next{};
	uint8_t keys[N]{};
	Voice voices[N]{};

	public:
	/**
	 * An iterator for going through all active voices.
	 */
	class Iterator {
		friend class VoiceManager;
		VoiceManager &manager;
		uint32_t i;

		Iterator(VoiceManager &manager, bool begin): manager(manager) {
			if (begin) {
				for(i = 0; i < N; ++i) {
					if (manager.keys[i]) {
						if (manager.voices[i].is_active())
							break;
						else
							manager.keys[i] = 0;
					}
				}
			} else {
				i = N;
			}
		}

		public:
		Voice &operator*() {
			return manager.voices[i];
		}

		Voice *operator->() {
			return &manager.voices[i];
		}

		bool operator==(const Iterator &other) {
			return i == other.i;
		}

		bool operator!=(const Iterator &other) {
			return !(*this == other);
		}

		Iterator &operator++() {
			while (i < N) {
				if (manager.keys[++i]) {
					if (manager.voices[i].is_active())
						break;
					else
						manager.keys[i] = 0;
				}
			}

			return *this;
		}
	};

	/**
	 * Get the voice for a given key.
	 *
	 * If no voice exists yet for the given key, activate a new voice.
	 * If there are no free voices, reuses the oldest activated voice.
	 *
	 * @param key  A MIDI key number.
	 * @return     The reference to a new voice.
	 */
	Voice &get(uint8_t key) {
		for(unsigned int i = 0; i < N; ++i) {
			if (keys[i] == key)
				return voices[i];
		}

		keys[next % N] = key;
		return voices[next++ % N];
	}

	/**
	 * Stops the voice for the given key.
	 *
	 * @param key  A MIDI key number.
	 */
	void stop(uint8_t key) {
		for(unsigned int i = 0; i < N; ++i) {
			if (keys[i] == key) {
				keys[i] = 0;
			}
		}
	}

	/**
	 * Returns an iterator to the first active voice.
	 *
	 * @return An iterator to the first active voice.
	 */
	Iterator begin() { return Iterator(*this, true); }

	/**
	 * Returns an iterator to the end of the active voices.
	 *
	 * @return An iterator to the end of the active voices.
	 */
	Iterator end() { return Iterator(*this, false); }
};

class Program {
	public:
	virtual ~Program() {};
	virtual void render(Chunk &chunk, float &zero_crossing) {};
	virtual void note_on(uint8_t key, uint8_t vel) {};
	virtual void note_off(uint8_t key, uint8_t vel) {};
	virtual void pitch_bend(int16_t bend) {};
	virtual void channel_pressure(int8_t pressure) {};
	virtual void poly_pressure(uint8_t key, uint8_t pressure) {};
	virtual void control_change(uint8_t control, uint8_t val) {};
};
