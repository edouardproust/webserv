#include <config/Config.hpp>
#include "constants.hpp"
#include "http/dev.http.hpp"
#include <iostream>

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}
	try {
		Config cfg(argv[1]);
		if (DEVMODE) {
			cfg.print();
			dev::runParserTests();
		}
		//Server server(cfg);
		//server.run(cfg);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
