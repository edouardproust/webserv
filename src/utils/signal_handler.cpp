#include <utils/signal_handler.hpp>

static bool ctrl_c_pressed = false;

static void signal_handler(int signal)
{
	ctrl_c_pressed = true;

	std::cout
		<< '\r'
		<< FT_WARNING
		<< "Signal " << signal << " (Ctrl + C) received."
		<< std::endl;
}

bool keep(void)
{
	static bool first_run = false;

	if (!first_run)
	{
		std::cout << FT_SETUP << "Setting up signals." << std::endl;

		signal(SIGINT, signal_handler);

		first_run = true;

		std::cout << FT_OK << "Signals ready." << std::endl;
		std::cout << std::endl;
	}

	return (!ctrl_c_pressed);
}