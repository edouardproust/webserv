#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include "colors.hpp"
#include <iostream>
#include <signal.h>

namespace sig {

	void handler(int signal);
	int keep(int action = -1);

}

#endif