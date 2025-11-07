#include "utils/signal_handler.hpp"
#include <iostream>
#include <signal.h>
#include "network/Colors.hpp"

void signal_handler(int signal)
{
    (void)signal;
    std::cout << "\r" << FT_STATUS << "Stopping Web server..." << std::endl;
    keep(0);
}

int keep(int action)
{
    static int keep = 1;

    if (action != -1)
        keep = action;

    if (keep == 1 && action == -1)
    {
        signal(SIGINT, signal_handler);
        signal(SIGQUIT, signal_handler);
    }

    return (keep);
}