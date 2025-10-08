#include <iostream>
#include "http/Request.hpp"
#include "http/RequestParser.hpp"

int main()
{
    RequestParser parser;
    Request request;

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
        "Two empty lines after headers - Valid"   //Extra lines considered as body
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
        "POST /index.html HTTP/1.0\r\nHost: localhost\r\n\r\n\r\n"
    };

    const int numTests = sizeof(rawRequests) / sizeof(rawRequests[0]);

 for (int i = 0; i < numTests; i++)
    {
        Request request;
        ParseStatus result = parser.parse_request(request, rawRequests[i]);
        std::cout << "Test " << (i + 1) << " (" << descriptions[i]
                  << ") -> result code: " << result << std::endl;
    }
    return 0;
}
