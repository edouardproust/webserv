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

	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 200 OK (welcome page)
	_onCatchRequest(srcPort, "GET /images/test.png HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found
	_onCatchRequest(srcPort, "GET /test/subdir/ HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found
	_onCatchRequest(srcPort, "GET /test/subdir/hello.jpeg HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found
	_onCatchRequest(srcPort, "GET /images/icons/ HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found
	_onCatchRequest(srcPort, "GET /nonexistentfile.html HTTP/1.1\r\nHost: localhost:8080\r\n\r\n"); // ✅ 404 Not Found
}
