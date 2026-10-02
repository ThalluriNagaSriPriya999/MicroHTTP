# MicroHTTP – Concurrent Web Server

A lightweight concurrent HTTP/1.1 web server developed in C using POSIX sockets, pthreads, Linux system calls, IPC, synchronization primitives, and secure file handling.

## 1. Project Overview

MicroHTTP is a custom HTTP web server designed to demonstrate operating-system and computer-networking concepts through a working system.

The server accepts TCP connections, places incoming clients into a synchronized request queue, processes requests using a fixed-size worker thread pool, serves static files, generates runtime monitoring information, records request logs, and communicates events to a separate monitoring process using IPC.

## 2. Problem Statement

Traditional web servers hide many operating-system mechanisms behind high-level frameworks.

MicroHTTP provides a low-level implementation that demonstrates how a web server can be built using fundamental Linux and POSIX mechanisms such as:

- TCP sockets
- Process creation
- Inter-process communication
- POSIX threads
- Mutexes
- Condition variables
- Signals
- File descriptors
- File-system operations
- HTTP request parsing
- Secure path validation
- Logging and monitoring

## 3. Objectives

The main objectives of MicroHTTP are:

1. Implement a working HTTP/1.1 web server in C.
2. Establish TCP client-server communication using POSIX sockets.
3. Process multiple clients concurrently using a thread pool.
4. Implement a bounded producer-consumer request queue.
5. Demonstrate IPC using a pipe and a separate monitoring process.
6. Serve static web resources from a controlled document root.
7. Protect the server against directory traversal attempts.
8. Reject oversized HTTP requests.
9. Implement graceful shutdown using SIGINT and SIGTERM.
10. Maintain request logs and runtime server statistics.
11. Provide automated unit tests for important modules.
12. Demonstrate practical operating-system concepts through a complete working system.

## 4. Major Features

### HTTP Server

- HTTP/1.0 and HTTP/1.1 request parsing
- GET request support
- HTTP response generation
- Content-Length headers
- MIME type detection
- Static file serving
- 404 Not Found handling
- 405 Method Not Allowed handling
- 400 Bad Request handling
- 413 Request Too Large handling
- 500 Internal Server Error handling

### Concurrency

- Fixed-size worker thread pool
- POSIX pthreads
- Bounded request queue
- Mutex synchronization
- Condition variables
- Producer-consumer design

### IPC

- Linux pipe-based IPC
- Parent server process
- Separate IPC monitoring process
- Connection events
- Request events
- Shutdown events

### Security

- Path validation
- Parent-directory traversal protection
- Backslash path rejection
- Control-character rejection
- Absolute HTTP path requirement
- Request-size limitation
- Controlled static-file document root

### Monitoring

- Total connections
- Total requests
- Successful requests
- Failed requests
- Active workers
- Queue size
- Server uptime
- HTML `/status` monitoring page

### Logging

Request logs include:

- Timestamp
- Worker ID
- Client IP address
- Client port
- HTTP method
- Requested path
- HTTP status code
- Response size

### Graceful Shutdown

The server handles:

- SIGINT
- SIGTERM

During shutdown it:

1. Stops accepting new clients.
2. Shuts down the request queue.
3. Stops worker threads.
4. Displays monitoring statistics.
5. Destroys synchronization resources.
6. Shuts down logging.
7. Stops the IPC monitor process.
8. Closes resources cleanly.

## 5. System Architecture

MicroHTTP follows a modular architecture:

    Client
       |
       v
    TCP Socket
       |
       v
    Server Accept
       |
       v
    Request Queue
       |
       v
    Worker Thread Pool
       |
       v
    HTTP Request Parser
       |
       +-------------------+
       |                   |
       v                   v
    Security           /status
    Validation          Handler
       |
       v
    Static File Handler
       |
       v
    HTTP Response
       |
       v
    Client

Parallel monitoring:

    Server / Workers
          |
          v
      IPC Pipe
          |
          v
    IPC Monitor Process

Supporting modules:

    Logger
    Monitor
    Signal Handler
    Request Queue
    Security
    Static File Handler

## 6. Thread Pool Architecture

MicroHTTP uses a fixed worker thread pool.

The server acts as the producer:

    Client
      |
      v
    accept()
      |
      v
    Request Queue

Worker threads act as consumers:

    Request Queue
      |
      +----> Worker 1
      +----> Worker 2
      +----> Worker 3
      +----> Worker 4

The queue is protected using:

- pthread_mutex_t
- pthread_cond_t

This prevents race conditions while multiple threads access the queue.

## 7. IPC Architecture

MicroHTTP uses a Linux pipe for inter-process communication.

The main server process creates the pipe and starts a monitoring child process.

Events sent through IPC include:

- Connection events
- Request events
- Shutdown events

Example:

    Main Server
        |
        | IPC pipe
        v
    Monitor Process

Example monitoring output:

    [IPC Monitor] CONNECTION from 127.0.0.1
    [IPC Monitor] REQUEST / -> HTTP 200
    [IPC Monitor] REQUEST /notfound.html -> HTTP 404
    [IPC Monitor] SHUTDOWN event received.

## 8. Security Design

MicroHTTP validates request paths before accessing the filesystem.

The security layer rejects:

- ../ traversal attempts
- Nested parent-directory traversal
- Backslash-based paths
- Control characters
- Relative paths

The static-file module also performs an additional parent-directory check before constructing the filesystem path.

The server additionally limits the size of an incoming HTTP request.

Example:

    curl -i --path-as-is http://127.0.0.1:8081/../../etc/passwd

The server returns:

    HTTP/1.1 404 Not Found

and does not expose the contents of `/etc/passwd`.

Oversized requests are rejected with:

    HTTP/1.1 413 Request Too Large

## 9. Project Structure

    MicroHTTP/
    |
    +-- src/
    |   +-- main.c
    |   +-- server.c
    |   +-- request_queue.c
    |   +-- thread_pool.c
    |   +-- http_parser.c
    |   +-- http_response.c
    |   +-- http_handler.c
    |   +-- static_file.c
    |   +-- security.c
    |   +-- logger.c
    |   +-- monitor.c
    |   +-- ipc.c
    |   +-- signal_handler.c
    |
    +-- include/
    |   +-- server.h
    |   +-- request_queue.h
    |   +-- thread_pool.h
    |   +-- http_parser.h
    |   +-- http_response.h
    |   +-- http_handler.h
    |   +-- static_file.h
    |   +-- security.h
    |   +-- logger.h
    |   +-- monitor.h
    |   +-- ipc.h
    |   +-- signal_handler.h
    |
    +-- tests/
    |   +-- test_http_parser.c
    |   +-- test_security.c
    |   +-- run_tests.sh
    |
    +-- www/
    |   +-- index.html
    |   +-- style.css
    |   +-- script.js
    |   +-- 404.html
    |
    +-- logs/
    +-- docs/
    +-- screenshots/
    +-- Makefile
    +-- README.md
    +-- .gitignore

## 10. Technologies Used

### Programming Language

- C11

### Operating System

- Linux
- Ubuntu on WSL2

### System APIs

- POSIX sockets
- pthreads
- fork()
- pipe()
- signal handling
- file descriptors
- filesystem APIs

### Networking

- TCP/IP
- HTTP/1.0
- HTTP/1.1

### Synchronization

- pthread mutex
- pthread condition variables
- bounded circular queue

### Development Tools

- GCC
- Make
- Git
- GitHub
- curl

## 11. Build Instructions

Clone the repository:

    git clone https://github.com/ThalluriNagaSriPriya999/MicroHTTP.git

Enter the project:

    cd MicroHTTP

Build the project:

    make

The project uses strict compiler flags:

    -std=c11
    -Wall
    -Wextra
    -Wpedantic
    -g

pthread support is enabled during linking.

## 12. Running the Server

Run with the default configuration:

    ./microhttp

Run on a specific port with a specific number of workers:

    ./microhttp 8081 4

Example:

    MicroHTTP listening on port 8081

The server can then be accessed using:

    http://127.0.0.1:8081/

## 13. HTTP Testing

### Successful request

    curl -i http://127.0.0.1:8081/

Expected:

    HTTP/1.1 200 OK

### Not found

    curl -i http://127.0.0.1:8081/notfound.html

Expected:

    HTTP/1.1 404 Not Found

### Unsupported method

    curl -i -X POST http://127.0.0.1:8081/

Expected:

    HTTP/1.1 405 Method Not Allowed

### Status page

    curl -i http://127.0.0.1:8081/status

The status page displays:

- Server uptime
- Total connections
- Total requests
- Successful requests
- Failed requests
- Active workers
- Queue size

## 14. Concurrency Testing

MicroHTTP was tested using 20 simultaneous requests:

    for i in {1..20}; do
        curl -s http://127.0.0.1:8081/ > /dev/null &
    done
    wait

The server successfully completed the concurrent requests using a four-worker thread pool.

The monitoring page reported:

    Total Connections : 25
    Total Requests    : 24
    Successful Requests: 22
    Failed Requests   : 2
    Active Workers    : 4
    Queue Size        : 0

The exact totals can vary depending on additional test requests performed during a server session.

## 15. Automated Testing

HTTP parser tests cover:

- Valid HTTP/1.1 GET request
- Valid HTTP/1.0 request
- Invalid request
- Unsupported HTTP version
- NULL request

Security tests cover:

- Root path
- Normal HTML path
- CSS path
- Parent-directory traversal
- Nested traversal
- Backslash path
- Relative path
- NULL path

Run the automated tests using:

    ./tests/run_tests.sh

Parser test result:

    Passed : 5
    Failed : 0

## 16. Graceful Shutdown Testing

Press:

    Ctrl+C

The server performs an orderly shutdown.

Example:

    Shutdown Signal Received
    Stopping new client connections...
    Stopping request queue...
    Stopping worker threads...
    Worker threads stopped.

    MicroHTTP Monitoring Stats

    Total Connections : 2
    Total Requests    : 2
    Successful        : 1
    Failed            : 1
    Active Workers    : 0
    Queue Size        : 0

    IPC monitor process stopped.
    MicroHTTP IPC shut down.

    MicroHTTP shutdown complete.

## 17. Operating System Concepts Demonstrated

MicroHTTP demonstrates several core operating-system concepts:

### Processes

    fork()

is used to create the IPC monitoring process.

### Threads

    pthread_create()

is used to create worker threads.

### Synchronization

Mutexes and condition variables protect the shared request queue.

### IPC

A Linux pipe is used for communication between the server process and monitoring process.

### Signals

SIGINT and SIGTERM are handled for graceful termination.

### File Descriptors

Sockets, pipes, and files are managed using Linux file descriptors.

### Resource Management

The server explicitly closes sockets, files, pipes, threads, mutexes, and condition variables during normal operation and shutdown.

## 18. Networking Concepts Demonstrated

MicroHTTP demonstrates:

- TCP socket creation
- bind()
- listen()
- accept()
- recv()
- send()
- HTTP request parsing
- HTTP response construction
- Content-Length
- MIME types
- Client-server communication

## 19. Error Handling

The server handles common failure conditions including:

- Socket creation failure
- bind failure
- listen failure
- accept interruption
- Invalid HTTP requests
- Unsupported HTTP methods
- Missing files
- Unsafe paths
- Oversized requests
- File read failures
- Response transmission failures
- Thread creation failures
- IPC failures

Errors are reported through standard error output and the logging subsystem where appropriate.

## 20. Future Enhancements

Possible future improvements include:

- HTTP keep-alive connections
- More complete HTTP header parsing
- URL percent-decoding before security validation
- HTTPS/TLS support
- Configuration files
- Dynamic content handling
- More extensive automated integration tests
- Performance benchmarking
- Rate limiting
- More detailed monitoring metrics

## 21. Project Status

Current implementation includes:

- TCP networking
- HTTP request parsing
- HTTP responses
- Static file serving
- Thread pool
- Request queue
- Mutex synchronization
- Condition variables
- IPC monitoring
- Security validation
- Request-size protection
- Logging
- Runtime monitoring
- Signal handling
- Automated parser tests
- Automated security tests
- Concurrent request testing
- Graceful shutdown

## 22. Author

MicroHTTP

Developed as an Operating Systems and Computer Networks project using C and Linux/POSIX system programming.

GitHub Repository:

https://github.com/ThalluriNagaSriPriya999/MicroHTTP
