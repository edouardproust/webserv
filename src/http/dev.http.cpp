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

void dev::runParserValidationTests()
{
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
		"POST with body but NO content-length header at all - Invalid",
		"Empty header value - Valid"
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
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\nHello",
		"POST /index.html HTTP/1.1\r\nHost: localhost:8080\r\nTest:  \t \r\n\r\n",
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

void dev::runParsedContentTests()
{
	RequestParser parser;
	
	struct ContentTest
	{
        const char* description;
        const char* rawRequest;
    };

	ContentTest tests[] =
	{
        {"Basic GET with query", "GET /search?q=hello&lang=fr HTTP/1.1\r\nHost: localhost\r\n\r\n"},
        {"Multiple query params", "GET /api/users?id=123&name=john&age=30 HTTP/1.1\r\nHost: localhost\r\n\r\n"},
        {"POST with body - JSON", "POST /submit HTTP/1.1\r\nHost: localhost\r\nContent-Length: 15\r\nContent-Type: application/json\r\n\r\n{\"key\":\"value\"}"},
        {"Header normalization", "GET / HTTP/1.1\r\nHOST: example.com\r\nCONTENT-TYPE: text/html\r\nUser-Agent: Test\r\n\r\n"},
        {"Mixed case headers", "GET /test HTTP/1.1\r\nHost: localhost\r\nX-Custom-Header: value\r\nX-ANOTHER-HEADER: anotherValue\r\n\r\n"},
        {"Path with special chars", "GET /files/123-abc_test.html HTTP/1.1\r\nHost: localhost\r\n\r\n"},
        {"Empty query", "GET /search? HTTP/1.1\r\nHost: localhost\r\n\r\n"},
        {"Complex URL", "GET /path/to/file?param1=value1&param2=value2&flag=true HTTP/1.1\r\nHost: localhost\r\nAccept: */*\r\n\r\n"},
        // OWS Trimming Tests
        {"OWS around header value - spaces", "GET / HTTP/1.1\r\nHost:   localhost  \r\nContent-Type:  text/html  \r\n\r\n"},
        {"OWS around header value - tabs", "GET / HTTP/1.1\r\nHost:\tlocalhost\t\r\nUser-Agent:\t\tTest\t\r\n\r\n"},
        {"OWS around header value - mixed", "GET / HTTP/1.1\r\nHost: \t localhost \t \r\nContent-Length:  11  \r\n\r\n"},
        {"Header with only OWS value", "GET / HTTP/1.1\r\nEmpty-Header:   \t  \r\nHost: localhost\r\n\r\n"}
    };

    const int numTests = sizeof(tests) / sizeof(tests[0]);

    for (int i = 0; i < numTests; i++)
	{
        std::cout << "\n=== Test " << (i + 1) << ": " << tests[i].description << " ===" << std::endl;
        std::cout << "Raw request: " << tests[i].rawRequest << std::endl;
        
        Request request;
        ParseStatus result = parser.parseRequest(request, tests[i].rawRequest);
        
        std::cout << "Parse status: " << parseStatusToString(result) << std::endl;
        
        if (result == PARSE_SUCCESS)
		{
            std::cout << "--- PARSED CONTENT ---" << std::endl;
            std::cout << request;
        }
		else
            std::cout << "--- PARSE FAILED ---" << std::endl;
        std::cout << "======================" << std::endl;
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
	headers3["Content-Length"] = "100";
	std::string emptyBody = "";
	std::cout << response.buildResponse(200, headers3, emptyBody) << std::endl;

	std::cout << "\n---Test 4: 500, with no Content-Type header---" << std::endl << std::endl ;
	std::map<std::string, std::string> noheaders;
	std::string testBody = "This is a test body";
	std::cout << response.buildResponse(500, noheaders, testBody) << std::endl;

	std::cout << "\n---Test 5: 200 OK, random header---" << std::endl << std::endl ;
	std::map<std::string, std::string> randomHeader;
	randomHeader["Random-Header"] = "Random value";
	std::string body3 = "Our Webserv is OP";
	std::cout << response.buildResponse(200, randomHeader, body3) << std::endl;

	std::cout << "\n---Test 6: 200 OK, random connection---" << std::endl << std::endl ;
	std::map<std::string, std::string> connHeader;
	connHeader["Connection"] = "Random";
	std::string body4 = "Connection is either keep-alive or close, else keep-alive by default";
	std::cout << response.buildResponse(200, connHeader, body4) << std::endl;
}
