#include "utils/signal.hpp"

// TODO
void	sig::handler(int signal)
{
	(void)signal;
	std::cout << "\r" << FT_STATUS << "Stopping Web server..." << std::endl;
	keep(0);
}

int	sig::keep(int action)
{
	static int keep = 1;

	if (action != -1)
		keep = action;

	if (keep == 1 && action == -1)
	{
		signal(SIGINT, handler);
		signal(SIGQUIT, handler);
	}

	return (keep);
}