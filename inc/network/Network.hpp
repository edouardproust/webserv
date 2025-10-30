#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "config/Config.hpp"
#include "router/Router.hpp"
#include "http/Response.hpp"

class Network {

	Config const& _config;

	// TODO make canonical
	Network();
	Network(Network const&);

	void	_onCatchRequest(HostPortPair const&, std::string const&) const;
	void	_sendResponse(HostPortPair const&, Response const& response) const;

	public:

		Network(Config const&);
		~Network();

		void	run() const;

};

#endif