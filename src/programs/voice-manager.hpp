/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>

#include "../pling.hpp"

/**
 * A manager for a fixed number of polyphonic voices.
 */
template <typename Voice, uint32_t N>
class VoiceManager
{
	bool sustain;

	/**
	 * The state for a voice.
	 */
	struct State {
		uint8_t key;
		bool active: 1;   /// Whether the voice is producing sound.
		bool pressed: 1;  /// Whether the key is pressed for this voice.
		bool sustained: 1; /// Whether the key was pressed while sustain is on.

		bool is_released()
		{
			return !pressed && !sustained;
		}
	} state[N] {};

	Voice voices[N] {};

public:
	/**
	 * An iterator for going through all active voices.
	 */
	class Iterator
	{
		friend class VoiceManager;
		VoiceManager &manager;
		uint32_t i;

		Iterator(VoiceManager &manager, bool begin): manager(manager)
		{
			if (begin) {
				for (i = 0; i < N; ++i) {
					if (manager.state[i].active) {
						if (manager.voices[i].is_active()) {
							break;
						} else {
							manager.state[i].active = false;
						}
					}
				}
			} else {
				i = N;
			}
		}

	public:
		Voice &operator*()
		{
			return manager.voices[i];
		}

		Voice *operator->()
		{
			return &manager.voices[i];
		}

		bool operator==(const Iterator &other)
		{
			return i == other.i;
		}

		bool operator!=(const Iterator &other)
		{
			return !(*this == other);
		}

		Iterator &operator++()
		{
			while (i < N) {
				if (manager.state[++i].active) {
					if (manager.voices[i].is_active()) {
						break;
					} else {
						manager.state[i].active = false;
					}
				}
			}

			return *this;
		}
	};

	/**
	 * Get the voice for a pressed key.
	 *
	 * If no voice exists yet for the given key, activate a new voice.
	 * If there are no free voices, try to reuse a non-pressed voice.
	 * If there are no free non-pressed voices, return nullptr.
	 *
	 * @param key  A MIDI key number.
	 * @return     A pointer to a voice, or nullptr if there is no suitable voice.
	 */
	Voice *press(uint8_t key)
	{
		unsigned int candidate = N;

		for (unsigned int i = 0; i < N; ++i) {
			if (state[i].key == key) {
				candidate = i;
				break;
			}

			if (!state[i].active && (candidate == N || state[candidate].active)) {
				candidate = i;
			} else if (candidate == N && !state[i].pressed) {
				candidate = i;
			}
		}

		if (candidate == N) {
			return nullptr;
		}

		state[candidate].key = key;
		state[candidate].active = true;
		state[candidate].pressed = true;
		state[candidate].sustained = sustain;

		return &voices[candidate];
	}

	/**
	 * Get the voice for a released key.
	 *
	 * @param key  A MIDI key number.
	 * @return     A pointer to a voice, or nullptr if there was no active voice.
	 */
	Voice *release(uint8_t key)
	{
		Voice *voice = nullptr;

		for (unsigned int i = 0; i < N; ++i) {
			if (state[i].active && state[i].key == key) {
				state[i].pressed = false;
				voice = &voices[i];

				if (!state[i].sustained) {
					voice->release();
				}

				break;
			}
		}

		return voice;
	}

	/**
	 * Release all voices.
	 */
	void release_all()
	{
		for (unsigned int i = 0; i < N; ++i) {
			state[i].pressed = false;
			voices[i].release();
		}
	}

	/**
	 * Enable or disable sustain.
	 *
	 * @param sustain  True if sustain should be on, false if off.
	 */
	void set_sustain(bool sustain)
	{
		this->sustain = sustain;

		if (sustain) {
			// Mark all pressed keys as being sustained
			for (unsigned int i = 0; i < N; ++i) {
				if (state[i].active && state[i].pressed) {
					state[i].sustained = true;
				}
			}
		} else {
			// Release all non-pressed sustained keys
			for (unsigned int i = 0; i < N; ++i) {
				if (state[i].active && state[i].sustained && !state[i].pressed) {
					voices[i].release();
				}

				state[i].sustained = false;
			}
		}
	}

	/**
	 * Stops the voice for the given key.
	 *
	 * @param key  A MIDI key number.
	 */
	void stop(uint8_t key)
	{
		for (unsigned int i = 0; i < N; ++i) {
			if (state[i].key == key) {
				state[i].active = false;
				state[i].pressed = false;
				state[i].sustained = false;
				voices[i].release();
			}
		}
	}

	/**
	 * Get the lowest note. Prefer pressed notes over active ones.
	 *
	 * @return A pointer to the lowest voice, or nullptr if there are no active voices.
	 */
	Voice *get_lowest()
	{
		unsigned int candidate = N;

		for (unsigned int i = 0; i < N; ++i) {
			if (!state[i].active) {
				continue;
			}

			if (candidate != N && state[i].key > state[candidate].key && state[i].is_released() >= state[candidate].is_released()) {
				continue;
			}

			if (candidate == N || !state[i].is_released() || state[candidate].is_released()) {
				candidate = i;
			}
		}

		return candidate < N ? &voices[candidate] : nullptr;
	}

	/**
	 * Returns an iterator to the first active voice.
	 *
	 * @return An iterator to the first active voice.
	 */
	Iterator begin()
	{
		return Iterator(*this, true);
	}

	/**
	 * Returns an iterator to the end of the active voices.
	 *
	 * @return An iterator to the end of the active voices.
	 */
	Iterator end()
	{
		return Iterator(*this, false);
	}
};
