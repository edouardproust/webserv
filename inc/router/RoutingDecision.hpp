#ifndef ROUTING_DECISION_HPP
#define ROUTING_DECISION_HPP

#include "config/LocationBlock.hpp"
#include <string>

enum RouteType {
    ROUTE_STATIC_OK,
	ROUTE_STATIC_LISTING,
    ROUTE_CGI,
    ROUTE_REDIRECT,
    ROUTE_ERROR,
	ROUTE_NONE
};

struct RoutingDecision {

	RouteType type;                 // e.g. ROUTE_STATIC_OK, ROUTE_CGI, ...
	int statusCode;                 // 200, 404, 301, etc.
	std::string targetPath;         // absolute path to file/script
	std::string redirectLocation;   // destination in case of redirection
	const LocationBlock* location;  // pointer to the matched location block, if any

	RoutingDecision()
		: type(ROUTE_ERROR),
			statusCode(500),
			targetPath(""),
			redirectLocation(""),
			location(NULL)
	{}

	RoutingDecision(RouteType t, int code, const std::string& path = "",
		const std::string& redirect = "", const LocationBlock* loc = NULL)
		: type(t), statusCode(code),
			targetPath(path),
			redirectLocation(redirect),
			location(loc)
	{}
	
};

#endif
