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
		Config cfg(argv[1]);
		/*
		Server server(cfg);
		server.run(cfg);

			// Inside server.run(), on each new request:
			Request request("GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n");
			int res = request.getHTTPStatus();
			if (res < 400) {
				Router router(request, cfg.getServers());
			} else {
				std::string const& errorPageContent = static::buildErrorPage(res);
				Response response(HTTP_VERSION, res, errorPageContent);
				server.sendResponseToClient(response);
			}
		*/
		if (DEVMODE) {
			std::cout << cfg << std::endl;
			Request request;
			request.parse("GET /images/test.jpg HTTP/1.0\r\nHost: localhost:8080\r\n\r\n");
			Router router(request, cfg.getServers(), HostPortPair("0.0.0.0:80")); // DEBUG using default ip:port pair until `network` module is done
			router.dispatchRequest();
			std::cout << router << std::endl;
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
