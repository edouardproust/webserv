#include "network/Network.hpp"

Network::Network(Config const& config): _config(config) {}

Network::~Network() {}

// TODO Add Daniel's code in this method
void	Network::run() const {
	// DEBUG Manual testing here :)
	HostPortPair listenedOn("localhost:80");
	_onCatchRequest(listenedOn, "GET /cgi-bin/testt.py HTTP/1.1\r\nHost: localhost:8080\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: 13\r\n\r\nname=John Doe");
	_onCatchRequest(listenedOn, "POST /cgi-bin/test.php HTTP/1.1\r\nHost: localhost:8080\r\nUser-Agent: WebservTest/1.0\r\n\r\n");
}

/**
 * Process a raw request when the Network module catches a raw request from a client.
 */
void Network::_onCatchRequest(HostPortPair const& srcPort, std::string const& rawRequest) const {
	Request request(rawRequest);
	std::string const& method = !request.getMethod().empty() ? (request.getMethod() + " ") : "";
	std::cout << "[info] Network: catched " << method << "request on port " << srcPort << std::endl;
	if (DEVMODE) std::cout << request << std::endl;
	Response const response = Router::dispatchRequest(_config, request, srcPort);
	if (DEVMODE) std::cout << response << std::endl;
	_sendResponse(srcPort, response); // TODO
}

// TODO Add Daniel's code in this method
void	Network::_sendResponse(HostPortPair const& destPort, Response const& response) const {
	std::cout << "[info] Network: sending '" << response.getStatus().toString() << "' response to client via port " << destPort << std::endl;
}
