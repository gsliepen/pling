/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "clock.hpp"

bool Clock::is_running() const
{
	return time_epoch != wall_clock::time_point{};
}


void Clock::start()
{
	if (is_running()) {
		return;
	}

	time_epoch = wall_clock::now();
}

void Clock::stop()
{
	if (!is_running()) {
		return;
	}

	auto now = wall_clock::now();
	std::chrono::duration<double> diff = now - time_epoch;
	beat_epoch += diff.count() * tempo / 60;
	time_epoch = {};
}

void Clock::reset()
{
	if (is_running()) {
		time_epoch = wall_clock::now();
	}

	beat_epoch = {};
}

double Clock::get_beat_position() const
{
	double beat = beat_epoch;

	if (is_running()) {
		auto now = wall_clock::now();
		std::chrono::duration<double> diff = now - time_epoch;
		beat += diff.count() * tempo / 60;
	}

	return beat;
}

double Clock::get_time_position() const
{
	return get_beat_position() * 60 / tempo;
}

Clock::Position Clock::get_position() const
{
	auto metre_beat = get_beat_position() / 4.0 * metre.lower;
	Position position;
	position.measure = metre_beat / metre.upper;
	position.beat = metre_beat - position.measure * metre.upper;
	return position;
}

void Clock::set_time_position(double time_position)
{
	time_epoch = wall_clock::now();
	beat_epoch = time_position * 60 / tempo;
}

void Clock::set_beat_position(double beat_position)
{
	time_epoch = wall_clock::now();
	beat_epoch = beat_position;
}

void Clock::set_position(Clock::Position position)
{
	time_epoch = wall_clock::now();
	beat_epoch = (position.measure * metre.upper + position.beat) * 4.0 / metre.lower;
}

float Clock::get_tempo() const
{
	return tempo;
}

void Clock::set_tempo(float tempo)
{
	if (is_running()) {
		auto now = wall_clock::now();
		std::chrono::duration<double> diff = now - time_epoch;
		beat_epoch += diff.count() * tempo / 60;
		time_epoch = now;
	}

	this->tempo = tempo;
}

Clock master_clock;
