/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

namespace Widgets {

class Widget {
	protected:
	float x{};
	float y{};
	float w{};
	float h{};

	public:
	Widget() {}
	virtual ~Widget() {}
	virtual void render(int screen_w, int screen_h) {};

	virtual Widget *get_clicked_widget(float x, float y) {
		return is_clicked(x, y) ? this : nullptr;
	}

	bool is_clicked(float x, float y) {
		return x >= this->x && x < this->x + w && y >= this->y && y < this->y + h;
	}

	void set_position(float x, float y, float w, float h) {
		this->x = x;
		this->y = y;
		this->w = w;
		this->h = h;
	};
};

}
