#include <iostream>
#include "server.hpp"
#include "config.hpp"

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	Config cfg;
	if (!cfg.load(argv[1])) {
		std::cerr << "Failed to load config" << std::endl;
		return 1;
	}

	try {
		Server server(cfg.port);
		server.run();
	} catch (const std::exception& e) {
		std::cerr << "Server error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

