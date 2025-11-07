#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <network/Colors.hpp>
#include <iostream>


#include <signal.h>

void signal_handler(int signal);
int keep(int action = -1);

#endif // SIGNAL_HANDLER_HPP