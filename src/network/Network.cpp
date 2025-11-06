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
 * Legend: ✅ Correct output, ❌ Incorrect output (need fixes)
 */
void	Network::_runManualTests() const {
	HostPortPair srcPort; // 0.0.0.0:80

	// --- config parsing errors ---
	_onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: 0.0.0.0:80\r\n\r\n");
	// missing root -> server { error_page 404 errors/404.html; }
	// missing root -> server { index index.htm; }

	// --- root directory ---
	// _onCatchRequest(srcPort, "GET / HTTP/1.1\r\nHost: 0.0.0.0:80\r\n\r\n");
	// // 200 OK (webserv welcome page) -> server {}
	// // 200 OK (/index.htm) -> server { root /home/edouard/Projects/webserv/tests/website; }
	// // 200 OK (autoindex) -> server { root /home/edouard/Projects/webserv/tests/website; index /invalid; }
	// // 403 Forbidden -(autoindex off) -> server { listen 80; root /home/edouard/Projects/webserv/tests/website; index /invalid; location / { autoindex off; } }

	// _onCatchRequest(srcPort, "GET /invalid HTTP/1.1\r\nHost: 0.0.0.0:80\r\n\r\n");

}
