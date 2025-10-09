# webserv

Description: *This project is about writing your own HTTP server. You will be able to test it with an actual browser. HTTP is one of the most widely used protocols on the internet. Understanding its intricacies will be useful, even if you won’t be working on a website.*

Subject: [click here](subject/en.subject.pdf)

Coworkers: [Skoteini-42](https://github.com/Skoteini-42), [devmarchesotti](https://github.com/devmarchesotti), [edouardproust](https://github.com/edouardproust)

## TODO

- Remove the virtual host form config parsing if not virtual host resolution is not implemented late on in the logic: remove `server_name` attribute and all the logic related in `ServerBlock`.

## How to use

1. Install dependencies
```
sudo apt-get update
sudo apt-get install php

2. Clone repo and build the program
```bash
git clone https://github.com/edouardproust/webserv.git webserv
cd webserv
make
```

3. Run the server
```bash
./webserv [configuration file]
```

## Project structure

**Modules**
- **`utils`**: [shared] Utils functions, like error and signal handling.
- **`server`:** [Daniel] Listen for TCP connections, read raw request, send raw response.
- **`http`:** [Ava] Parse raw request into Request object and build raw response.
- **`config`:** parse configuration file into a structured Config object.
- **`router`:** Determine if request is for static content or CGI.
- **`static`:** [Ava] Read static files and produce raw HTTP response.
- **`cgi`:** [Edouard] Execute CGI program and retrieve output.

**Mandatory Updates**
- **HTML1.0 to HTML 1.1:** will concern `server` (keep-alive connexions) and `http` modules (and `config`?)
- **Implement signals:** will be spread over the project

**Global logic**
- `main` init `Config` object (based on program argument) then start, run and stop server.
- `server` is the starting point of the program (infinite loop listening for requests).
- On request catch, `server` performs several actions. Quick example:
	```cpp
	std::string raw_request = get_request(socket);
	Request request = parse_request(std::string); // module 'http'
	std:string plain_response;
	if (is_static(req)) // module `router`
		raw_response = process_static(req); // module 'static'
	else
		raw_response = process_cgi(req); // module 'cgi'
	Reponse response = parse_response(); // module 'http'
	send_response(socket, response);
	```

### Allowed functions used per module

**util**
- `strerror`, `errno`, `gai_strerror` → error handling
- `signal`, `kill` → signal handling for shutdown/interrupts

**server**

- `socket` → create server socket
- `bind` → bind socket to address/port
- `listen` → set socket to listen mode
- `accept` → accept client connection
- `connect` → connect to a remote server (useful if implementing proxy/forwarding)
- `recv` → read bytes from socket
- `send` → write bytes to socket
- `close` → close socket
- `select`, `poll`, `epoll` (`epoll_create`, `epoll_ctl`, `epoll_wait`) → handle multiple simultaneous connections
- `kqueue`, `kevent` → BSD alternative to epoll for event-driven I/O
- `socketpair` → create pair of connected sockets (used sometimes in IPC or special cases in servers)
- `fcntl` → set socket to non-blocking mode
- `setsockopt` → configure socket options (`SO_REUSEADDR`, timeouts, etc.)
- `getsockname` → get the local address/port of a socket
- `htons`, `htonl`, `ntohs`, `ntohl` → network/host byte conversion for ports and addresses
- `getaddrinfo`, `freeaddrinfo` → hostname resolution if needed
- `getprotobyname` → resolve protocol (e.g., "tcp")

**http**

- `std::string` functions (`str.find`, `str.substr`...) → Parsing

**config**

- `open`, `read`, `close` → open config file, read its contents, close it
- `access` → check existence of files/directories referenced in config
- `stat` → file/directory information (root, cgi-bin, error pages)
- `opendir`, `readdir`, `closedir` → optionally for directory indexes or listings

**router**

- `access` → check if file exists
- `stat` → check if path is file or directory

**static**

- `open`, `read`, `close` → open requested static file, read its content, close it
- `stat` → get file size for Content-Length
- `access` → check file permissions
- `opendir`, `readdir`, `closedir` → for directory listings or index.html handling

**cgi**

- `pipe` → create parent/child communication channels for stdin/stdout
- `fork` → create child process
- `dup2`, `dup` → redirect child stdin/stdout to pipe
- `chdir` → change working directory to the script’s folder in the child process, so relative paths in the script resolve correctly
- `execve` → execute CGI script (php-cgi, Python, etc.)
- `write` → send POST body to CGI
- `read` → read CGI output
- `close` → close unused pipe ends
- `waitpid` → wait for child process
