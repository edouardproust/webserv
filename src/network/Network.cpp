#include "network/Network.hpp"

Network::Network(Config const& config): _config(config) {}

Network::~Network() {}

// TODO Add Daniel's code in this method
void	Network::run() const {
	std::vector<HostPortPair> const& portsToOpen = _config.getAllListenPorts();
	for (size_t i = 0; i < portsToOpen.size(); ++i)
		std::cout << "[INFO] " << SERVER_SOFTWARE << ": Listening on port " << portsToOpen[i] << std::endl;
	_runManualTests(); // DEBUG: Remove in prod
}

/**
 * Process a raw request when the Network module catches a raw request from a client.
 */
void Network::_onCatchRequest(HostPortPair const& srcPort, std::string const& rawRequest) const {
	Request request(rawRequest);
	std::string const& method = !request.getMethod().empty() ? (request.getMethod() + " ") : "";
	std::cout << "[INFO] Network: catched " << method << "request on port " << srcPort << std::endl;
	if (DEVMODE) std::cout << request << std::endl;
	Response const response = Router::dispatchRequest(_config, request, srcPort);
	if (DEVMODE) std::cout << response << std::endl;
	_sendResponse(srcPort, response); // TODO
}

// TODO Add Daniel's code in this method
void	Network::_sendResponse(HostPortPair const& destPort, Response const& response) const {
	std::cout << "[INFO] Network: sending '" << response.getStatus().toString() << "' response to client via port " << destPort << std::endl;
}

/**
 * This method is for testing during developement process.
 * Legend:
 * - ✅ Correct output
 * - ❌ Incorrect output (need fixes)
 */
void	Network::_runManualTests() const {
	HostPortPair srcPort("localhost:80");

	// --- BASIC ROUTES ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 200 OK (website index OR webserv default index if [location "/" > directive "root"] is commented)
	_onCatchRequest(srcPort, "GET /index.html?query=test HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 200 OK (website index)
	_onCatchRequest(srcPort, "GET /dir-index HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 200 OK (index file in /dir-index/ OR index.html if [location "/dir-listing/" > directive "index"] is commented )
	_onCatchRequest(srcPort, "GET /dir-listing HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 200 OK (autoindex listing) -> // TODO (ava) StaticHandler: _serveAutoindex()
	_onCatchRequest(srcPort, "GET /dir-off HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 403 Forbidden (autoindex off, no index file)
	_onCatchRequest(srcPort, "GET /doesnotexist HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found (custom errors/404.html page)

	// --- METHOD TESTS ---
	_onCatchRequest(srcPort, "POST /cgi/test.php?name=John%20Doe&age=23 HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 12\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nname=webserv"); // ✅ 200 OK
	_onCatchRequest(srcPort, "DELETE /uploads/test.txt HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 204 No Content (file deleted)
	// limit_except
	_onCatchRequest(srcPort, "GET /uploads/test.txt HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 405 Method Not Allowed
	// Unsupported method
	_onCatchRequest(srcPort, "PUT /cgi/test.php HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 501 Not Implemented
	_onCatchRequest(srcPort, "TRACE / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 501 Not Implemented
	// Invalid method
	_onCatchRequest(srcPort, "INVALID /cgi/test.php HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 400 Bad Request

	// --- MORE CGI TESTS ---
	// .php
	_onCatchRequest(srcPort, "GET /cgi/nonexistent.php HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found (php-cgi cannot find script)
	_onCatchRequest(srcPort, "GET /cgi/invalid.php HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 500 Internal Server Error (php-cgi fails to parse script with syntax error)
	_onCatchRequest(srcPort, "POST /cgi/test.php HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 20\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nusername=test&pwd=42"); // ✅ 200 OK (with POST parameters)
	//.py
	_onCatchRequest(srcPort, "GET /cgi/nonexistent.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 500 Internal Server Error (python3 fails to execute non-existing script)
	_onCatchRequest(srcPort, "GET /cgi/env.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 200 OK (prints environment variables correctly)
	_onCatchRequest(srcPort, "GET /cgi/timeout.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ❌ 200 OK (should timeout and return 504 Gateway Timeout) // TODO (Ed) Implement timeout on CGI execution
	_onCatchRequest(srcPort, "GET /cgi/segfault.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 500 Internal Server Error (process killed by signal 11)

	// --- BAD REQUESTS ---
	_onCatchRequest(srcPort, "GOT / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 400 Bad Request (invalid method)
	_onCatchRequest(srcPort, "GET / HTTP/1.0\r\n\r\n"); // ❌ 400 Bad Request (invalid version) -> Should be 505 Not supported // TODO (Ava)
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\n\r\n"); // ✅ 400 Bad Request (missing Host header)
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 10\r\n\r\n"); // TODO (Ava) do the test
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\n\r\n"); // TODO (Ava) do the test
	_onCatchRequest(srcPort, "POST / HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: abc\r\n\r\n"); //  ❌ 200 OK -> Should be 400 bad request (invalid CL). // TODO (Ava) Please check the type and overflow of other attributes when necessary (eg. int)
	_onCatchRequest(srcPort, "POST /uploads HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 12\r\nTransfer-Encoding: chunked\r\n\r\nHello world!"); // ✅ 400 Bad Request (both Content-Length and Transfer-Encoding)

	// --- BODY SIZE / LIMIT TESTS ---
	std::string bigBody(2000000, 'A'); // 2MB
	_onCatchRequest(srcPort, "POST /uploads HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 2000000\r\n\r\n" + bigBody); // ✅ 413 Content Too Large

	// --- REDIRECTION / ERROR PAGE TESTS ---
	_onCatchRequest(srcPort, "GET /redirect HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 301 Moved Permanently
	_onCatchRequest(srcPort, "GET /doesnotexist HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ Custom 404 page

	// --- SECURITY TESTS ---
	_onCatchRequest(srcPort, "GET /../webserv.config HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ❌ 200 OK -> Should be 403 Forbidden (directory traversal) // TODO (Ed)

	_onCatchRequest(srcPort, "GET /../../../../ HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ❌ 200 OK (autoindex) -> Should be 403 Forbidden // TODO (Ed)

	// --- HEADER STRESS TEST ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: WebservTest\r\nUser-Agent: Duplicate\r\nAccept: */*\r\n\r\n"); // ✅ 200 OK (similar headers are overwritten: acceptable behaviour)
	_onCatchRequest(srcPort, "POST /cgi/test.php HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 0\r\n\r\n"); // ✅ 200 OK

	// --- PIPELINED REQUESTS ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\nGET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ❌ 411 Lenght Required -> Should be 400 Bad Request (pipelining not supported) // TODO (Ava)
}
