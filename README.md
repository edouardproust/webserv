# webserv

Description: *This project is about writing your own HTTP server. You will be able to test it with an actual browser. HTTP is one of the most widely used protocols on the internet. Understanding its intricacies will be useful, even if you won’t be working on a website.*

Subject: [click here](subject/en.subject.pdf)

Coworkers: [Skoteini-42](https://github.com/Skoteini-42), [devmarchesotti](https://github.com/devmarchesotti), [edouardproust](https://github.com/edouardproust)

## How to use

```bash
./webserv [configuration file]
```

## Project structure

**Modules**
- **util**: [shared] Utils functions, like error and signal handling.
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
- `main` calls `server`
- `server` is the starting point of the program (infinite loop listening for requests).
- On request catch, `server` performs several actions:
	```cpp
	std::string plain_req = get_request(socket);
	Request req = parse_request(std::string); // module 'http'
	std:string plain_res;
	if (is_static(req)) // module `router`
		plain = process_static(req); // module 'static'
	else
		plain = process_cgi(req); // module 'cgi'
	Reponse res = parse_response(); // module 'http'
	send_response(socket);
	```

### Allowed functions used per module

**`util`**
- `strerror` / `errno` → error handling
- `signal` / `kill` → signal handling for shutdown/interrupts

**server**

- `socket` → create server socket
- `bind` → bind socket to address/port
- `listen` → set socket to listen mode
- `accept` → accept client connection
- `recv` → read bytes from socket
- `send` → write bytes to socket
- `close` → close socket
- `poll` / `select` / `epoll` → handle multiple simultaneous connections
- `fcntl` → set socket to non-blocking mode
- `htons`, `htonl`, `ntohs`, `ntohl` → network/host byte conversion for ports and addresses
- `getaddrinfo` / `freeaddrinfo` → hostname resolution if needed

**http**

- `std::string` functions (`str.find`, `str.substr`...) → Parsing

**config**

- `open` → open config file
- `read` → read file contents
- `close` → close file
- `access` → check existence of files/directories referenced in config
- `stat` → file/directory information (root, cgi-bin, error pages)
- `opendir` / `readdir` / `closedir` → optionally for directory indexes or listings

**router**

- `access` → check if file exists
- `stat` → check if path is file or directory

**static**

- `open` → open requested file
- `read` → read file content
- `close` → close file
- `stat` → get file size for Content-Length
- `access` → check file permissions
- `opendir` / `readdir` / `closedir` → for directory listings or index.html handling

**cgi**

- `pipe` → create parent/child communication channels for stdin/stdout
- `fork` → create child process
- `dup2` → redirect child stdin/stdout to pipe
- `execve` → execute CGI script (php-cgi, Python, etc.)
- `write` → send POST body to CGI
- `read` → read CGI output
- `close` → close unused pipe ends
- `waitpid` → wait for child process
