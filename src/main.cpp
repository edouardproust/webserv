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
			try {
				Request request("GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n");
			} catch (const std::exception& e) {
				std::string const& errorPageContent = static::buildErrorPage(res);
				Response response(HTTP_VERSION, res, errorPageContent);
				server.sendResponseToClient(response);
			}
			Router router(request, cfg.getServers());

			OR:

			Request request("GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n");
			if (request.getParsingStatus() != PARSE_SUCCESS) {
				std::string const& errorPageContent = static::buildErrorPage(res);
				Response response(HTTP_VERSION, res, errorPageContent);
				server.sendResponseToClient(response);
			}
			Router router(request, cfg.getServers());
		*/
		if (DEVMODE) {
			std::cout << cfg << std::endl;
			Request request;
			request.parse("GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n");
			Router router(request, cfg.getServers());
			router.print(); //TODO refactor using operator<<
		}
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
