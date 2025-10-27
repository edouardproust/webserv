#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <network/Colors.hpp>
#include <iostream>


#include <signal.h>

void (*signal(int sig, void (*func)(int)))(int);
bool keep(void);

#endif // SIGNAL_HANDLER_HPP