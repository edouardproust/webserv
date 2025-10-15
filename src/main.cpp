#include <config/Config.hpp>
#include "Constants.hpp"
#include "http/dev.http.hpp"
#include "router/Router.hpp"
#include <iostream>

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}
	try {
		Config cfg(argv[1]);
		if (DEVMODE) {
			//cfg.print();
			//dev::runParserValidationTests();
			//dev::runParsedContentTests();
			dev::runResponseTests();
			//Request request = dev::parseRequest("GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n");
			//Router router(request, cfg.getServers());
			//router.print();
		}
		//Server server(cfg);
		//server.run(cfg);
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
