#ifndef WEBSERVER_HPP
#define WEBSERVER_HPP

#include "Socket.hpp"
#include "Colors.hpp"

#include <sys/epoll.h>
#include <map>
#include <vector>
#include <string>
#include <iostream>

namespace ft
{
    class WebServer {
    private:
        int _epoll;
        std::vector<ft::Socket*> _connections;
        std::map<int, std::string> _request_list;  // fd -> accumulated request
        std::map<int, std::string> _response_list; // fd -> remaining response to send

    public:
        WebServer(const std::vector<t_server_config> &confs);
        ~WebServer();

        void epollInit();
        void epollAddServers();
        void start();

        void handle_recv(int client_fd, struct epoll_event &event);
        void handle_send(int client_fd, struct epoll_event &event);

    private:
        bool is_request_complete(const std::string &buf);
        int get_expected_length(const std::string &buf);
        std::string build_simple_response(const std::string &request);
    };
}

#endif
