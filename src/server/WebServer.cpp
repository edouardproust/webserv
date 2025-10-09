#include "WebServer.hpp"
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <sstream>


WebServer::WebServer(const std::vector<t_server_config> &confs)
{
    for (std::vector<t_server_config>::const_iterator it = confs.begin(); it != confs.end(); ++it)
    {
        ft::Socket* s = new ft::Socket(*it);
        s->bindSocket();
        s->listenSocket();
        _connections.push_back(s);
    }

    std::cout << FT_SETUP << "WebServer configured with " << _connections.size() << " socket(s)." << std::endl;
}

WebServer::~WebServer()
{
    for (std::vector<ft::Socket*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
        delete *it;
    if (_epoll >= 0)
        close(_epoll);
    std::cout << FT_CLOSE << "WebServer stopped." << std::endl;
}

void WebServer::epollInit()
{
    _epoll = ::epoll_create(1);
    if (_epoll < 0)
        throw std::runtime_error("epoll_create failed");
    std::cout << FT_OK << "Epoll created." << std::endl;
}

void WebServer::epollAddServers()
{
    struct epoll_event ev;
    for (std::vector<ft::Socket*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
    {
        ev.events = EPOLLIN;
        ev.data.fd = (*it)->getSock();
        if (::epoll_ctl(_epoll, EPOLL_CTL_ADD, (*it)->getSock(), &ev) < 0)
            throw std::runtime_error(std::string("epoll_ctl add server failed: ") + strerror(errno));
        std::cout << FT_OK << "Added server socket fd " << (*it)->getSock() << " to epoll." << std::endl;
    }
}

/* helper: check if headers end and if Content-Length satisfied */
bool WebServer::is_request_complete(const std::string &buf)
{
    size_t header_end = buf.find("\r\n\r\n");
    if (header_end == std::string::npos)
        return false;

    int content_len = get_expected_length(buf);
    if (content_len <= 0)
        return true;

    size_t body_len = buf.length() - (header_end + 4);
    return (int)body_len >= content_len;
}

int WebServer::get_expected_length(const std::string &buf)
{
    size_t pos = buf.find("Content-Length:");
    if (pos == std::string::npos)
        pos = buf.find("Content-length:");
    if (pos == std::string::npos)
        return 0;
    pos += strlen("Content-Length:");
    while (pos < buf.size() && (buf[pos] == ' ' || buf[pos] == '\t'))
        ++pos;
    size_t end = buf.find("\r\n", pos);
    if (end == std::string::npos)
        return 0;
    std::string val = buf.substr(pos, end - pos);
    std::istringstream iss(val);
    int length = 0;
    iss >> length;
    if (iss.fail())
        return 0;
    return length;
}

std::string WebServer::build_simple_response(const std::string &request)
{
    (void)request;
    std::string body = "Hello World\n";
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Content-Type: text/plain\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    return oss.str();
}

void WebServer::handle_recv(int client_fd, struct epoll_event &event)
{
    char buffer[4096];
    while (1)
    {
        ssize_t bytes = ::recv(client_fd, buffer, sizeof(buffer), 0);
        if (bytes > 0)
        {
            _request_list[client_fd].append(buffer, (size_t)bytes);
        }
        else if (bytes == 0)
        {
            ::epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, 0);
            close(client_fd);
            _request_list.erase(client_fd);
            _response_list.erase(client_fd);
            std::cout << FT_EVENT << "Client fd " << client_fd << " closed connection." << std::endl;
            return;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            ::epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, 0);
            close(client_fd);
            _request_list.erase(client_fd);
            _response_list.erase(client_fd);
            std::cout << FT_STATUS << "recv() error on fd " << client_fd << ": " << strerror(errno) << std::endl;
            return;
        }
    }

    if (is_request_complete(_request_list[client_fd]))
    {
        std::cout << FT_EVENT << "Complete request received on fd " << client_fd
                  << " (" << _request_list[client_fd].size() << " bytes)." << std::endl;
        std::string resp = build_simple_response(_request_list[client_fd]);
        _response_list[client_fd] = resp;

        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.fd = client_fd;
        if (::epoll_ctl(_epoll, EPOLL_CTL_MOD, client_fd, &ev) < 0)
        {
            ::epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, 0);
            close(client_fd);
            _request_list.erase(client_fd);
            _response_list.erase(client_fd);
            std::cout << FT_STATUS << "epoll_ctl MOD failed for fd " << client_fd << std::endl;
            return;
        }
    }
}

void WebServer::handle_send(int client_fd, struct epoll_event &event)
{
    (void)event;
    std::map<int, std::string>::iterator it = _response_list.find(client_fd);
    if (it == _response_list.end())
    {
        ::epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, 0);
        close(client_fd);
        return;
    }

    std::string &resp = it->second;
    ssize_t total = (ssize_t)resp.size();
    ssize_t sent = 0;

    while (sent < total)
    {
        ssize_t n = ::send(client_fd, resp.data() + sent, (size_t)(total - sent), 0);
        if (n > 0)
            sent += n;
        else if (n == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            ::epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, 0);
            close(client_fd);
            _request_list.erase(client_fd);
            _response_list.erase(client_fd);
            std::cout << FT_STATUS << "send() error on fd " << client_fd << ": " << strerror(errno) << std::endl;
            return;
        }
    }

    if (sent >= total)
    {
        _response_list.erase(it);
        _request_list.erase(client_fd);
        ::epoll_ctl(_epoll, EPOLL_CTL_DEL, client_fd, 0);
        close(client_fd);
        std::cout << FT_OK << "Response sent and connection closed for fd " << client_fd << std::endl;
    }
    else
    {
        resp.erase(0, (size_t)sent);
    }
}

void WebServer::start()
{
    epollInit();
    epollAddServers();

    struct epoll_event events[32];

    while (1)
    {
        int nfds = epoll_wait(_epoll, events, 32, -1);
        if (nfds < 0)
        {
            if (errno == EINTR)
                continue;
            throw std::runtime_error("epoll_wait failed");
        }

        for (int i = 0; i < nfds; ++i)
        {
            int fd = events[i].data.fd;

            bool is_server_socket = false;
            for (std::vector<ft::Socket*>::iterator it = _connections.begin(); it != _connections.end(); ++it)
            {
                if (fd == (*it)->getSock())
                {
                    is_server_socket = true;
                    break;
                }
            }

            if (is_server_socket)
            {
                int client_fd = ::accept(fd, 0, 0);
                if (client_fd >= 0)
                {
                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLET;
                    ev.data.fd = client_fd;
                    epoll_ctl(_epoll, EPOLL_CTL_ADD, client_fd, &ev);
                    std::cout << FT_EVENT << "Accepted new client fd " << client_fd << std::endl;
                }
            }
            else
            {
                if (events[i].events & EPOLLIN)
                    handle_recv(fd, events[i]);
                else if (events[i].events & EPOLLOUT)
                    handle_send(fd, events[i]);
            }
        }
    }
}
