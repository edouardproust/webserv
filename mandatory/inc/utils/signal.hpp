#ifndef SIGNAL_HANDLER_HPP
#define SIGNAL_HANDLER_HPP

#include <csignal>
#include <iostream>

namespace sig {

    void setup();
    bool keepRunning();

}

#endif
