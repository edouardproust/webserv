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

	// TODO canonical ?
	Client(Client const&);
	Client&	operator=(Client const&);

	public:

		Client(int, Socket*, Network*);
		~Client();

		void	updateActivity();
		void	setCloseAfterResponse(bool);
		void	closeSocket();
		void	resetForNextRequest();
		void	prepareResponseSend(Response const&);
		bool	continueResponseSend();
		bool	isInactive(time_t, size_t) const;

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
