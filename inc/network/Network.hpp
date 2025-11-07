#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "router/Router.hpp"
#include "network/Socket.hpp"
#include "utils/signal.hpp"
#include <sys/epoll.h>

class Network {

	private:
		Config const& _config;
		std::vector<Socket *> _connections;
		std::map<int, std::string> _request_list;
		std::map<int, Socket*> _client_server_map;

		int _epoll;

		void epoll();
		void epollAddServers();
		int epoll_wait(struct epoll_event *events);

		int isServerSideEvent(int epoll_fd);
		void recv(int client_fd, struct epoll_event &events_setup);
		void send(int client_fd, struct epoll_event &events_setup);

		void _handleClientDisconnect(int client_fd, struct epoll_event &events_setup);
		void _handleRecvError(int client_fd, struct epoll_event &events_setup);

		int			getRequestTotalLength(std::string request);
		std::string getBoundry(std::string request);

	public:
		Network();
		Network(Config const& _config_file);
		~Network();
		void start_servers();
		int getEpollFd() const;
    	const std::vector<Socket*>& getConnections() const;
    	const std::map<int, std::string>& getRequestList() const;
   	 	const std::map<int, Socket*>& getClientServerMap() const;

		class EpollException : public std::exception {
			public:
				const char *what() const throw();
		};

		class EpollCtlException : public std::exception {
			public:
				const char *what() const throw();
		};

		class EpollWaitException : public std::exception {
			public:
				const char *what() const throw();
		};
};

std::ostream& operator<<(std::ostream& os, const Network& rhs);

#endif // WEBSERVER_HPP