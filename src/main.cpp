#include <config/Config.hpp>
#include "constants.hpp"
#include "http/dev.http.hpp"
#include "router/Router.hpp"
#include <iostream>

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	try {
		Config config(argv[1]);

		// To be moved into 'network' listening loop:
			std::cout << config << std::endl;
			Request request("GE /images/hello.jpg HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
			if (DEVMODE) std::cout << request << std::endl;
			Response const response = Router::dispatchRequest(config, request, HostPortPair("localhost:8080")); // DEBUG using default ip:port pair until `network` module is done
			if (DEVMODE) std::cout << response << std::endl;
			//Network::_sendResponse(response);

	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
