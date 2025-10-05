#include <config/Config.hpp>
#include <iostream>

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	Config cfg(argv[1]);
	try {
		cfg.parse();
		cfg.print(); // DEBUG
	} catch (const std::exception& e) {
		std::cerr << "Config error: " << e.what() << std::endl;
		return 1;
	}

	/*
	try {
		Server server(cfg.port);
		server.run();
	} catch (const std::exception& e) {
		std::cerr << "Server error: " << e.what() << std::endl;
		return 1;
	}
	*/

	return 0;
}

