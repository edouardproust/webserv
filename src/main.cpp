#include <config/Config.hpp>
#include "http/RequestParser.hpp"
#include <iostream>
#include <network/Network.hpp>
#include <network/Socket.hpp>

const char* parseStatusToString(ParseStatus status)
{
    switch (status)
    {
        case PARSE_SUCCESS:
            return ("OK");
        case PARSE_ERR_BAD_REQUEST:
            return ("PARSE_ERR_BAD_REQUEST");
        case PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED:
            return ("PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED");
        case PARSE_ERR_HEADER_NAME_EMPTY:
            return ("PARSE_ERR_HEADER_NAME_EMPTY");
        case PARSE_ERR_HEADER_SYNTAX_ERROR:
            return ("PARSE_ERR_HEADER_SYNTAX_ERROR");
        default:
            return ("UNKNOWN");
    }
}

void runParserTests()
{
    RequestParser parser;

    const char* descriptions[] =
    {
        "GET with body - Valid", //GET doesn't need a body, but it is allowed to have one
        "Chars before method in request line - Invalid", //Nothing should be before method, even spaces
        "No empty line at end of headers - Invalid",
        "Method lowercase - Invalid",
        "Path without starting '/' - Invalid",
        "Empty raw request - Invalid",
        "Path missing - Invalid",
        "No spaces in request line - Invalid",
        "Wrong HTTP version - Invalid",
        "Multiple spaces in request line - Valid",
        "Tabs instead of spaces - Invalid",
        "Path with special chars - Valid",
        "Extra spaces after version - Valid", //only whitespaces valid-everything else invalid even tabs
        "Empty line after request line - Invalid", //here strictly following RFC, nginx allows it but doesn't make sense
        "Two empty lines after headers - Valid",   //Extra lines considered as body
        "Spaces after header name - Invalid",
        "Spaces after headers - Valid"
    };

    const char* rawRequests[] =
    {
        "GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\nhost: value\twith\ttabs\r\n\r\nHello world",
        "     GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n",
        "GET /index.html HTTP/1.0\r\nHost: localhost:8080\r\n",
        "get /index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n",
        "GET index.html HTTP/1.0\r\nHost: localhost:8080\r\n\r\n",
        "",
        "GET HTTP/1.0\r\nHost: localhost:8080\r\n\r\n",
        "GET/index.htmlHTTP/1.0\r\nHost: localhost:8080\r\n\r\n",
        "GET /index.html HTTP/4.0\r\nHost: localhost:8080\r\n\r\n",
        "GET    /index.html    HTTP/1.0\r\nHost: localhost\r\n\r\n",
        "GET\t/index.html\tHTTP/1.0\r\nHost: localhost\r\n\r\n",
        "GET /index123&.html HTTP/1.0\r\nHost: localhost\r\n\r\n",
        "GET /index.html HTTP/1.0   \r\nHost: localhost\r\n\r\n",
        "GET /index.html HTTP/1.0\r\n\r\nHost: localhost\r\n\r\n",
        "POST /index.html HTTP/1.0\r\nHost: localhost\r\n\r\n\r\n",
        "POST /index.html HTTP/1.0\r\nHost\t   : localhost\r\n\r\n",
        "POST /index.html HTTP/1.0\r\nHost: localhost    \t \v   \r\n\r\n"
    };

    const int numTests = sizeof(rawRequests) / sizeof(rawRequests[0]);
    
    for (int i = 0; i < numTests; i++)
    {
        Request request;
        ParseStatus result = parser.parse_request(request, rawRequests[i]);
         std::cout << "Test " << (i + 1) << ": " << descriptions[i] << " - "
            << (result == PARSE_SUCCESS ? "PASS" : "FAIL") 
            << " (" << parseStatusToString(result) << ")" << std::endl;
    }
}

int main(int argc, char** argv) {

	// if (argc == 2 && std::string(argv[1]) == "--test-parser") {
    //     runParserTests();
    //     return 0;
    // }

	// if (argc < 2) {
	// 	std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
	// 	return 1;
	// }

	// try {
	// 	Config cfg(argv[1]);
	// 	cfg.print(); // DEBUG
	// 	//Server server(cfg);
	// 	//server.run(cfg);
	// } catch (const std::exception& e) {
	// 	std::cerr << "Error: " << e.what() << std::endl;
	// 	return 1;
	// }
    (void)argv;
    (void)argc;

    std::vector<t_server_config> configs;
    t_server_config cfg;
    cfg.port = "8080";
    cfg.server_names.push_back("localhost");
    configs.push_back(cfg);

    try
    {
        Network server(configs);
        server.start();
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ERROR] " << e.what() << std::endl;
        return 1;
    }

	return 0;
}
