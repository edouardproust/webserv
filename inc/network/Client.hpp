#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "network/Socket.hpp"
#include "http/Response.hpp"
class Network;

// TODO calss message with canonical explanation
class Client
{
	int			_fd;
	Socket*		_socket;
	Network*	_network;
	time_t		_lastActivity;

	Request		_pendingRequest;
	std::string	_pendingResponse;
	size_t		_responseSendPos;
	bool		_shouldCloseAfterResponse;

	std::string	_remoteAddr;

	static size_t const	_CLOSE_TIMEOUT_SECONDS;
	static size_t const	_KEEPALIVE_TIMEOUT_SECONDS;

	void	_resetForNextRequest();

	// TODO canonical ?
	Client(Client const&);
	Client&	operator=(Client const&);

	public:

		Client(int, Socket*, Network*);
		~Client();

		void	updateActivity();
		void	closeSocket();
		void	prepareResponseSend(Response const&);
		bool	continueResponseSend();
		bool	isInactive(time_t) const;

		int				getFd() const;
		Socket*			getSocket() const;
		time_t			getLastActivity() const;
		Request const&		getRequest() const;
		Request&			getRequest();
		std::string const&	getResponse() const;
		size_t const&		getResponseSendPos() const;
		bool			shouldCloseAfterResponse() const;
		std::string const&	getRemoteAddr() const;

		void	setRemoteAddr(const std::string&);

};

#endif
