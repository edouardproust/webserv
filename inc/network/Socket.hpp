#ifndef SOCKET_HPP
#define SOCKET_HPP

#include "Colors.hpp"

#include <string>
#include <vector>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

struct t_server_config {
        std::string port;
        std::vector<std::string> server_names;
};

class Socket {
    private:
        int                 _sock;
        struct addrinfo*    _servinfo;
        t_server_config     _server;
        struct addrinfo     _hints;

    public:
        Socket(const t_server_config &server);
        ~Socket();

        void    setAddrStruct(void);
        void    loadAddressInfo(void);
        void    createSocket(void);
        void    bindSocket(void);
        void    listenSocket(void);
        int     acceptConnection(void);
        int     getSock(void) const;
};

#endif
