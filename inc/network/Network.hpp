
#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include <dirent.h>
#include <sys/epoll.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <vector>
#include "config/Config.hpp"
#include "network/Socket.hpp"

#define FT_DEFAULT_CONFIG_FILE "./configurations/webserv.conf"
#define FT_DEFAULT_CLIENT_BUFFER_SIZE 1024
#define FT_MAX_EVENT_SIZE 100

	class Network
	{
	private:
		Config _config_file;
		std::vector<Socket *> _connections;
		std::map<int, std::string> _request_list;
		int _epoll;
		bool _is_sudo;

		void epoll();
		void epollAddServers();
		int epoll_wait(struct epoll_event *events);

		void checkSudo();

		int isServerSideEvent(int epoll_fd);
		void recv(int client_fd, struct epoll_event &events_setup);
		void send(int client_fd, struct epoll_event &events_setup);

		int			getRequestTotalLength(std::string request);
		std::string getBoundry(std::string request);

	public:
		Network(const char *configuration_file);
		~Network();
		void start_servers();

		class EpollException : public std::exception
		{
		public:
			const char *what() const throw();
		};

		class EpollCtlException : public std::exception
		{
		public:
			const char *what() const throw();
		};

		class EpollWaitException : public std::exception
		{
		public:
			const char *what() const throw();
		};

		class cantGetUserInfoException : public std::exception
		{
		public:
			const char *what() const throw();
		};

		class needSudoException : public std::exception
		{
		public:
			const char *what() const throw();
		};
	};

#endif // WEBSERVER_HPP