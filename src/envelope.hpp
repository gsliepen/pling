/* SPDX-License-Identifier: GPL-3.0-or-later */

#pragma once

namespace Envelope {

class ADSR {
	float amplitude{};
	float t{-1};

	public:
	struct Parameters {
		float attack;
		float decay;
		float sustain;
		float release;
	};

	void init() {amplitude = {}; t = {};}
	bool is_active() {return t >= 0;}
	float update(Parameters &param, bool on);
};

}
