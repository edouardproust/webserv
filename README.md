# webserv

Description: *This project is about writing your own HTTP server. You will be able to test it with an actual browser. HTTP is one of the most widely used protocols on the internet. Understanding its intricacies will be useful, even if you won’t be working on a website.*

Subject: [click here](subject/en.subject.pdf)

Coworkers: [Skoteini-42](https://github.com/Skoteini-42), [edouardproust](https://github.com/edouardproust)

https://github.com/user-attachments/assets/30e12151-7202-4fca-ac31-ef7d638bd1f1

## Table of Contents

- [How to use](#how-to-use)
- [Features](#features)
- [Configuration Syntax](#configuration-syntax)
- [Used methods per module](#used-methods-per-module)
- [Resources](#-resources)

## How to use

### 1. Install dependencies
```
sudo apt-get update
sudo apt-get install php
```

### 2. Clone repo and build the program

```bash
git clone https://github.com/edouardproust/webserv.git webserv
cd webserv
make
```

### 3. Tests

**Production environment**

Non-verbose logs in `log/access.log` and `log/error.log` for `docker` comptiblity, no `valgrind` tests:
```
make test_prod
```
Using a custom config file:
```
make && ./webserv [config_file]
```

**Development environment**

Verbose logs in the terminal with `valgrind` tests:
```
make test_dev
```

**42 tester**

Stress tests without `valgrind`:
```
make test_42
```

**Custom test**

## Features

### HTTP Protocol
- Full HTTP/1.1 compliance (RFC 9110, RFC 9112)
- Support for persistent connections keep-alive or close if specified
- Chunked transfer encoding
- Multiple request methods: GET, POST, PUT, DELETE, HEAD

### Configuration
- Custom configuration file parsing
- Virtual hosts with multiple server blocks
- Location-based routing with regex support
- Custom error pages
- Client body size limits
- Autoindex for directory listings

### CGI Support
- **Multiple scripting languages**: PHP (.php) and Python (.py)
- **Custom extensions**: Configurable via `cgi` directive (e.g., .bla)
- **Environment variables**: Full CGI/1.1 environment support
- **Timeout handling**: Configurable CGI execution timeouts
- **Security**: Proper script validation and execution

### Session Management
- Cookie-based session handling
- Session creation, validation, and destruction
- Session timeout support

### Redirection
- Full 3xx redirect support: 301, 302, 307, 308
- Method preservation (307/308 for API compatibility)
- Configurable via `return` directive

### Static File Serving
- MIME type detection
- Directory indexing (autoindex)

3. Run the server
```bash
./webserv [configuration file]
```

## Configuration Syntax

### Structure

```nginx
server {
    # Server directives
    listen host:port [host2:port2 ...];
    server_name name1 [name2 ...];
    root /absolute/path;

    location /path {
        # Location directives
    }
}
```

### Server Directives

| Directive | Syntax | Default | Description |
|-----------|--------|---------|-------------|
| `listen` | `host:port` \| `port` \| `host` | `0.0.0.0:80` | Listening address(es). Use `localhost` or IPv4. Omit host for `0.0.0.0`, omit port for `80` |
| `server_name` | `name1 [name2 ...]` | - | Virtual host names for request matching |
| `root` | `/absolute/path` | - | **Required.** Document root (must be absolute) |
| `client_max_body_size` | `size[K\|M\|G]` | `1G` | Max request body size (e.g., `10M`, `1G`) |
| `upload_store` | `path` | - | Upload directory for PUT requests (absolute or relative to root) |
| `index` | `file1 [file2 ...]` | `index.html index.htm` | Index files for directories |
| `error_page` | `code [...] /path` | - | Custom error pages (codes 300-599) |

### Location Directives

| Directive | Syntax | Default | Description |
|-----------|--------|---------|-------------|
| `allowed_methods` | `GET [POST PUT ...]` | All methods | Whitelist of allowed HTTP methods |
| `return` | `code [url]` | - | Return status or redirect (url required for 3xx codes) |
| `autoindex` | `on` \| `off` | `on` | Enable directory listing |
| `client_max_body_size` | `size[K\|M\|G]` | Server value | Override server limit |
| `upload_store` | `path` | Server value | Override server upload directory |
| `index` | `file1 [file2 ...]` | Server value | Override server index files |
| `error_page` | `code [...] /path` | Server value | Override server error pages |
| `cgi` | `.ext /path/to/executor` | - | CGI handler (e.g., `cgi .py /usr/bin/python3`) |

### Notes

- Comments start with `#`
- Paths in `location` blocks inherit server `root`
- Relative paths resolved against server `root`
- PUT requires `upload_store` to be enabled
- Multiple servers on same `host:port` require unique `server_name` for virtual hosting
- Location paths must be absolute (e.g., `/`, `/api`, `/static`)

### Example

```nginx
server {
    listen 8080 localhost:8081;
    server_name example.com;
    root /var/www/html;
    client_max_body_size 10M;
    error_page 404 /errors/404.html;

    location / {
        index index.html;
        autoindex on;
        allowed_methods GET POST;
    }

    location /uploads {
        allowed_methods GET POST PUT DELETE;
        upload_store /var/uploads;
        client_max_body_size 100M;
    }

    location /cgi-bin {
        allowed_methods GET POST;
        cgi .py /usr/bin/python3;
        cgi .php /usr/bin/php-cgi;
    }

    location /redirect {
        return 301 https://example.com;
    }
}
```

## Used methods per module

### `network`

Handles TCP socket creation, client connections, and non-blocking I/O multiplexing using epoll/kqueue for concurrent request handling.

- `socket` → create server socket
- `bind` → bind socket to address/port
- `listen` → set socket to listen mode
- `accept` → accept client connection
- `connect` → connect to a remote server (useful if implementing proxy/forwarding)
- `recv` → read bytes from socket
- `send` → write bytes to socket
- `close` → close socket
- `epoll_create`, `epoll_ctl`, `epoll_wait` → handle multiple simultaneous connections. Register, modify and delete fds from epoll surveillance.
- `kqueue`, `kevent` → BSD alternative to epoll for event-driven I/O
- `socketpair` → create pair of connected sockets (used sometimes in IPC or special cases in servers)
- `fcntl` → set socket to non-blocking mode
- `setsockopt` → configure socket options (`SO_REUSEADDR`, timeouts, etc.)
- `getsockname` → get the local address/port of a socket
- `htons`, `htonl`, `ntohs`, `ntohl` → network/host byte conversion for ports and addresses
- `getaddrinfo`, `freeaddrinfo` → hostname resolution if needed
- `getprotobyname` → resolve protocol (e.g., "tcp")

### `config`

Parses nginx-style configuration files and validates server/location blocks, directives, and file paths.

- `open`, `read`, `close` → open config file, read its contents, close it
- `access` → check existence of files/directories referenced in config
- `stat` → file/directory information (root, cgi-bin, error pages)
- `opendir`, `readdir`, `closedir` → optionally for directory indexes or listings

### `http`

Parses HTTP/1.1 requests (headers, body, chunked encoding) and builds compliant HTTP responses.

- `std::string` functions (`str.find`, `str.substr`...) → Parsing

### `router`

Routes incoming requests to appropriate handlers based on URI matching and location block configuration.

- `access` → check if file exists
- `stat` → check if path is file or directory


### `cgi`

Executes CGI scripts (PHP, Python) in child processes with pipe-based communication for stdin/stdout redirection.

- `pipe` → create parent/child communication channels for stdin/stdout
- `fork` → create child process
- `dup2`, `dup` → redirect child stdin/stdout to pipe
- `chdir` → change working directory to the script’s folder in the child process, so relative paths in the script resolve correctly
- `execve` → execute CGI script (php-cgi, Python, etc.)
- `write` → send POST body to CGI
- `read` → read CGI output
- `close` → close unused pipe ends
- `waitpid` → wait for child process
- `fcntl` → set pipes to non-blocking mode
- `epoll_ctl` → register pipes fd to epoll for surveillance, and unregister them.

### `static`

Serves static files with MIME type detection, directory indexing, and proper Content-Length headers.

- `open`, `read`, `close` → open requested static file, read its content, close it
- `stat` → get file size for Content-Length
- `access` → check file permissions
- `opendir`, `readdir`, `closedir` → for directory listings or index.html handling

### `utils`

Provides logs, signal management, and helper functions used across all modules.

- `strerror`, `errno`, `gai_strerror` → error handling
- `signal`, `kill` → signal handling for shutdown/interrupts

## 📚 Resources

- https://www.rfc-editor.org/rfc/rfc9112.html
- https://www.rfc-editor.org/rfc/rfc9110
- https://www.rfc-editor.org/rfc/rfc3875.html#section-4.1.1
- https://en.wikipedia.org/wiki/HTTP
- https://http.dev/400
- https://developer.mozilla.org/en-US/docs/Web/HTTP/Guides/
- https://stackoverflow.com/
- https://github.com/nginx/nginx
