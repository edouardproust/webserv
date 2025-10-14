#include "http/dev.http.hpp"
#include <iostream>

Request const&	dev::parseRequest(std::string const& rawRequest) {
	static RequestParser parser;
	static Request request;
	ParseStatus result = parser.parseRequest(request, rawRequest);
	if (result != PARSE_SUCCESS) {
		throw std::runtime_error("Error parsing request: " + parseStatusToString(result));
	}
	return (request);
}

std::string	dev::parseStatusToString(ParseStatus status) {
	switch (status) {
		case PARSE_SUCCESS:
			return "OK";
		case PARSE_ERR_BAD_REQUEST:
			return "PARSE_ERR_BAD_REQUEST";
		case PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED:
			return "PARSE_ERR_HTTP_VERSION_NOT_SUPPORTED";
		case PARSE_ERR_LENGTH_REQUIRED:
			return "PARSE_ERR_LENGTH_REQUIRED";
		default:
			return ("UNKNOWN");
	}
}

void dev::runParserTests() {
	RequestParser parser;

	const char* descriptions[] = {
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
		"Spaces after header name (before colon)- Invalid",
		"Spaces after headers - Valid",
		"HTTP 1.1 without host header - Invalid",
		"POST with no body and without content-length header - Valid", //since there is no body, content-length header is not required
		"POST with body but wrong content-length header value - Invalid",
		"POST with body but NO content-length header at all - Invalid"
	};

	const char* rawRequests[] = {
		"GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 11\r\nhost: value\twith\ttabs\r\n\r\nHello world",
		"     GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n",
		"GET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n",
		"get /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"GET index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"",
		"GET HTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"GET/index.htmlHTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"GET /index.html HTTP/4.0\r\nHost: localhost:8080\r\n\r\n",
		"GET    /index.html    HTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"GET\t/index.html\tHTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"GET /index123&.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"GET /index.html HTTP/1.1   \r\nHost: localhost:8080\r\n\r\n",
		"GET /index.html HTTP/1.1\r\n\r\nHost: localhost:8080\r\n\r\n",
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 2\r\n\r\n\r\n",
		"POST /index.html HTTP/1.1\r\nHost\t   : localhost:8080\r\nContent-Length: 0\r\n\r\n",
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 0   \r\n\r\n",
		"POST /index.html HTTP/1.1\r\n\r\nContent-Length: 0\r\n\r\n",
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n",
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 7\r\n\r\nHello",
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\nHello"
	};

	const int numTests = sizeof(rawRequests) / sizeof(rawRequests[0]);
	for (int i = 0; i < numTests; i++) {
		Request request;
		::ParseStatus result = parser.parseRequest(request, rawRequests[i]);
		std::cout << "Test " << (i + 1) << ": " << descriptions[i] << " - "
			<< (result == PARSE_SUCCESS ? "PASS" : "FAIL")
			<< " (" << parseStatusToString(result) << ")" << std::endl;
	}
}

void dev::runResponseTests()
{
    Response response;
    
	std::cout << "---Test 1: 200 OK, with body---" << std::endl << std::endl ;
    std::map<std::string, std::string> headers1;
    headers1["Content-Type"] = "text/html";
	std::string body1 = "<html><body><h1>Hello World</h1></body></html>";
    std::string response200 = response.buildResponse(200, headers1, body1);
    std::cout << response200 << std::endl;
    
	std::cout << "\n---Test 2: 404, with body---" << std::endl << std::endl ;
    std::map<std::string, std::string> headers2;
    headers2["Content-Type"] = "text/html";
	std::string body2 = "<html><body><h1>404 Not Found</h1></body></html>";
    std::string response404 = response.buildResponse(404, headers2, body2);
    std::cout << response404 << std::endl;

	std::cout << "\n---Test 3: 200 OK, with no body---" << std::endl << std::endl ;
	std::map<std::string, std::string> headers3;
	headers3["Content-Type"] = "text/plain";
	std::string emptyBody = "";
	std::cout << response.buildResponse(200, headers3, emptyBody) << std::endl;

	std::cout << "\n---Test 4: 500, with no Content-Type header---" << std::endl << std::endl ;
	std::map<std::string, std::string> noheaders;
	std::string testBody = "This is a test body";
	std::cout << response.buildResponse(500, noheaders, testBody) << std::endl;
}
