#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "http/HttpStatus.hpp"
#include "router/RoutingDecision.hpp"
#include "cgi/CgiData.hpp"
#include "utils/utils.hpp"
#include <ctime>

/**
 * Represents an HTTP response with status, headers, and body.
 *
 * Provides utilities to build, modify, stringify, and parse raw HTTP responses.
 * Value-type class following the orthodox canonical form.
 */
class Response
{
	HttpStatus					_status;
	UniqHeaders					_headers;
	std::vector<std::string>	_setCookieHeaders;
	std::string					_body;
	bool						_bodyClearedForHead;
	std::string					_servedFilePath; // for debug purpose only
	bool						_needsCgiExecution;
	CgiData*					_cgiData; // Owning (deleted in destructor);
	std::string					_pendingSessionUsername;
	bool						_expireSession;

	void		_initDefaultHeaders();
	void		_updateContentLength();
	void		_manageContentType();
	std::string	_buildStatusLine() const;
	void		_parseRawResponse(std::string const&);
	bool		_hasHeader(std::string const&) const;
	std::string	_buildHeaders() const;

	public:

 		// Othodox canonical form
		Response();
		Response(std::string const&);
		Response(Response const&);
		Response& operator=(Response const&);
		~Response();

		std::string stringify(bool = false) const;
		void		clearBody();
		void		clearBodyForHead();
		bool		isConnectionClose() const;

		static Response	initCgiResponse(RoutingDecision const&, std::string const&, HostPortPair const&);
		void			parseFromCgiOutput(std::string const&);
		void			parseHeadersFromCgiOutput(std::string const&);
		CgiData*		transferCgiDataOwnership();

		void	setStatus(HttpStatus const&);
		void	setHeader(std::string const&, std::string const&);
		void	setBodyAndContentLength(std::string const&);
		void	setServedFilePath(std::string const&);
		void	setConnectionFromRequest(Request const&);
		void	addSetCookieHeader(const std::string& name, const std::string& value, 
                   const std::string& options);

		HttpStatus const& 				getStatus() const;
		UniqHeaders const&				getHeaders() const;
		const std::vector<std::string>&	getSetCookieHeaders() const;
		std::string const&				getBody() const;
		const std::string&				getPendingSessionUsername() const;
		std::string const&				getServedFilePath() const;
		bool							needsCgiExecution() const;
		CgiData const*					getCgiData() const;

		void							handleSession(const Request& request);

		class RawException: public std::runtime_error {
			public:
				RawException(std::string const&);
		};
};

std::ostream& operator<<(std::ostream&, Response const&);

#endif
