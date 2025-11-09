#include "utils/signal.hpp"

static volatile sig_atomic_t running = 1;

static void handler(int)
{
    running = 0; // signal-safe
}

void sig::setup()
{
    signal(SIGINT, handler);
    signal(SIGQUIT, handler);
}

bool sig::keepRunning()
{
    return running != 0;
}
