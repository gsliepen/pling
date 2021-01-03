/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

#include <cstdint>
#include <chrono>

/**
 * The master clock.
 */
class Clock
{
public:
	using wall_clock = std::chrono::steady_clock;

	struct Metre {
		unsigned int upper;
		unsigned int lower;
	};

	struct Position {
		std::int32_t measure;
		float beat;
	};

	void start();
	void stop();
	void reset();

	double get_time_position() const;
	double get_beat_position() const;
	Position get_position() const;
	float get_tempo() const;

	void set_time_position(double time_position);
	void set_beat_position(double beat_position);
	void set_position(Position position);
	void set_tempo(float tempo);


private:
	bool is_running() const;

	wall_clock::time_point time_epoch{};
	double beat_epoch{};
	float tempo{120};
	Metre metre{4, 4};
};

extern Clock master_clock;
