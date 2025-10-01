# webserv

Description: *This project is about writing your own HTTP server. You will be able to test it with an actual browser. HTTP is one of the most widely used protocols on the internet. Understanding its intricacies will be useful, even if you won’t be working on a website.*

Subject: [click here](subject/en.subject.pdf)

Coworkers: [Skoteini-42](https://github.com/Skoteini-42), [devmarchesotti](https://github.com/devmarchesotti), [edouardproust](https://github.com/edouardproust)

## Daniel – Networking & Core

* Set up socket, listen, accept.
* Handle multiple connections (poll/epoll).
* Manage client buffers (reading/writing).

In summary : produce “raw request strings” and send back “raw response strings.”

## Foteini – HTTP Protocol & Requests

* Parse HTTP requests into objects (method, path, headers).
* Build HTTP responses with headers and body.
* Implement GET/POST/DELETE logic.

In summary: make a raw request string → parse it → build a response string.

## Edouard – Config & Features

* Parse config file into structured data.
* Implement per-server/location settings (roots, error pages, etc.).
* Add CGI handling.

In summary: provide helper functions like “given a request, find the right root/error page/CGI program.”

## Project modules / structure

webserv\
|\
|_ server\
|_ http\
|_ cgi\
|_ config\
|_ signals\

## How to use

```bash
./webserv [configuration file]
```

## Allowed functions

All functionalities must be implemented in C++ 98.

### Process Control & Program Execution (For handling multiple client requests)
* `fork`: Creates a new process to handle a client connection concurrently, allowing the main server to continue accepting new connections.
* `execve`: Replaces the current process image (e.g., to run a CGI script like PHP or Perl).
* `waitpid`: Allows the server to clean up finished child processes ("zombies") to free up system resources.

### Inter-Process Communication (IPC) - Pipes (Mainly for CGI)
* `pipe`: Creates a channel for communication between the web server and a CGI script (e.g., to send data to the script and read its output).
* `dup` / `dup2`: Redirects standard input (stdin) and output (stdout) of a CGI script to/from the pipes created by the server.

### Signal Handling (For graceful server shutdown and management)
* `signal`: Catches signals like SIGINT (Ctrl-C) to allow the server to shut down gracefully (close all sockets, clean up child processes).
* `kill`: Sends signals to child processes, for example, to terminate a long-running CGI script.

### File & Directory Management (For serving static files)
* `open` / `close`: Open and close files requested by clients (e.g., HTML, CSS, JPEG files).
* `read` / `write`: Read the content of a file to send it in an HTTP response, or write uploaded data to a new file.
* `access` / `stat`: Check if a file exists, is readable, and get its size (for the `Content-Length` header) and modification date (for caching headers).
* `chdir`: Change the current working directory to the web root directory for security.
* `opendir` / `readdir` / `closedir`: List the contents of a directory to generate an automatic directory listing if no index file is present.

### Error Handling (For robust error reporting)
* `errno` / `strerror`: Get and display human-readable error messages when system calls (like `open`, `accept`) fail.
* `gai_strerror`: Get human-readable error messages for failures in hostname lookup (`getaddrinfo`).

### Network Core (The foundation of the web server)

#### Byte Order Conversion (For correct network communication)
* `htons` / `htonl`: Convert port numbers and addresses from the host's byte order to network byte order before binding a socket.
* `ntohs` / `ntohl`: Convert network data back to the host's byte order after receiving it.

#### Socket Creation and Configuration
* `socket`: Creates the main listening socket that will wait for incoming HTTP connections.
* `setsockopt`: Crucial for setting the `SO_REUSEADDR` option, allowing the server to restart immediately without waiting for the previous socket to timeout.
* `fcntl`: Used to set sockets to **non-blocking** mode, which is essential for efficient I/O multiplexing with `poll`/`epoll`/`kqueue`.
* `getsockname`: Finds out which port the server is actually listening on if it was configured to use a random port (port 0).

#### Address Resolution (For binding to a host and port)
* `getaddrinfo` / `freeaddrinfo`: The modern way to prepare the server's address structure for `bind()`. It handles both IPv4 and IPv6 transparently.

### HTTP Protocol Engine (Handling connections and requests)

#### Connection Management (TCP Lifecycle)
* `bind`: Assigns the server's IP address and port number (e.g., port 80) to the listening socket.
* `listen`: Puts the socket into a state where it can accept incoming TCP connections from HTTP clients (browsers).
* `accept`: Accepts an incoming connection, creating a new socket dedicated to communication with that specific client.
* `recv`: Reads the HTTP request (e.g., `GET /index.html HTTP/1.1`) sent by the client.
* `send`: Sends the HTTP response (headers and file content) back to the client.
* `connect`: *Less common in a server*, but could be used if the server itself acts as a client (e.g., a proxy server).

### High-Performance Concurrency (Handling thousands of simultaneous clients)

This is the most critical part for a scalable web server.

#### I/O Multiplexing (The heart of the event loop)
* **`select` / `poll`**: The portable way to monitor the **listening socket** and all active **client sockets** simultaneously. The server sleeps until one or more sockets are ready (e.g., a new connection arrives or a client sends data). This avoids the need for one thread/process per client.
* **`epoll` (Linux) / `kqueue` (macOS/BSD)**: Superior, modern alternatives to `select`/`poll`. They are dramatically more efficient when handling tens of thousands of concurrent connections, as they only return the sockets that are actually ready, without scanning the entire list.
