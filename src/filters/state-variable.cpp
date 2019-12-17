/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "state-variable.hpp"

namespace Filter {

bool StateVariable::Parameters::build_widget(const std::string &name) {
	(void)name;
	return false;
}

}
