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
 */
void	Network::_runManualTests() const {
	HostPortPair srcPort("localhost:80");

	// --- BASIC ROUTES ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
	_onCatchRequest(srcPort, "GET /index.html?query=test HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
	_onCatchRequest(srcPort, "GET /doesnotexist HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // 404 expected
	_onCatchRequest(srcPort, "GET /dir/ HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // autoindex test

	// --- METHOD TESTS ---
	_onCatchRequest(srcPort, "POST /cgi-bin/test.php?name=John%20Doe&age=23 HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 12\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nname=webserv");
	_onCatchRequest(srcPort, "POST /cgi-bin/test.php HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 0\r\n\r\n"); // empty body + content-length 0
	_onCatchRequest(srcPort, "DELETE /upload/test.txt HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // test DELETE route
	// Unsupported method
	_onCatchRequest(srcPort, "PUT /cgi-bin/test.php HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // expect 405
	_onCatchRequest(srcPort, "TRACE / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // expect 405 or 501
	// Invalid method
	_onCatchRequest(srcPort, "INVALID /cgi-bin/test.php HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // expect 400

	// --- BAD REQUESTS ---
	_onCatchRequest(srcPort, "GOT / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // invalid method
	_onCatchRequest(srcPort, "GET / HTTP/1.0\r\n\r\n"); // HTTP/1.0 without Host header
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\n\r\n"); // missing Host -> 400
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 10\r\n\r\n"); // missing body but CL present
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\nTransfer-Encoding: chunked\r\n\r\n"); // not supported encoding
	_onCatchRequest(srcPort, "POST / HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: abc\r\n\r\n"); // invalid CL

	// --- BODY SIZE / LIMIT TESTS ---
	std::string bigBody(2000000, 'A'); // 2MB
	_onCatchRequest(srcPort, "POST /upload HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 2000000\r\n\r\n" + bigBody);

	// --- CGI TESTS ---
	_onCatchRequest(srcPort, "GET /cgi-bin/env.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // affiche variables d'env
	_onCatchRequest(srcPort, "POST /cgi-bin/form.php HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 19\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nusername=test&pwd=42");
	_onCatchRequest(srcPort, "GET /cgi-bin/timeout.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // test de timeout CGI
	_onCatchRequest(srcPort, "GET /cgi-bin/segfault.py HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // script qui crashe -> 500

	// --- REDIRECTION / ERROR PAGE TESTS ---
	_onCatchRequest(srcPort, "GET /redirect HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // expect 301 or 302
	_onCatchRequest(srcPort, "GET /error404 HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // custom 404

	// --- SECURITY TESTS ---
	_onCatchRequest(srcPort, "GET /../secret.txt HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // directory traversal
	_onCatchRequest(srcPort, "GET /cgi-bin/../../etc/passwd HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");

	// --- HEADER STRESS TEST ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: WebservTest\r\nUser-Agent: Duplicate\r\nAccept: */*\r\n\r\n"); // duplicate headers
	_onCatchRequest(srcPort, "POST /cgi-bin/test.php HTTP/1.1\r\nHost: localhost:8080\r\nContent-Length: 0\r\n\r\n"); // empty body POST

	// --- PIPELINED REQUESTS ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\nGET /index.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");
}
