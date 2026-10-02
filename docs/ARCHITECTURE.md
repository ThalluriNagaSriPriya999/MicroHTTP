MICROHTTP — COMPLETE SYSTEM ARCHITECTURE

MicroHTTP is a modular, concurrent HTTP/1.1 web server developed in C using
POSIX sockets, pthreads, Linux system calls, IPC, synchronization and secure
filesystem operations.


                         ┌─────────────────────────┐
                         │      HTTP CLIENT        │
                         │                         │
                         │ Browser / curl / Client │
                         └────────────┬────────────┘
                                      │
                                      │ TCP Connection
                                      ▼
                    ┌─────────────────────────────────┐
                    │       NETWORKING LAYER           │
                    │                                 │
                    │ socket()                        │
                    │ bind()                          │
                    │ listen()                        │
                    │ accept()                        │
                    └───────────────┬─────────────────┘
                                    │
                                    │ Client Connection
                                    ▼
                    ┌─────────────────────────────────┐
                    │        REQUEST QUEUE             │
                    │                                 │
                    │ Bounded Circular Queue          │
                    │ Capacity: 64                    │
                    │                                 │
                    │ pthread_mutex                   │
                    │ pthread_cond_wait()              │
                    │ pthread_cond_signal()            │
                    │                                 │
                    │ Producer → Main Server          │
                    │ Consumer → Worker Threads       │
                    └───────────────┬─────────────────┘
                                    │
                                    │
             ┌──────────────────────┼──────────────────────┐
             │                      │                      │
             ▼                      ▼                      ▼
      ┌─────────────┐        ┌─────────────┐        ┌─────────────┐
      │   WORKER 1  │        │   WORKER 2  │  ...   │   WORKER 4  │
      │   pthread   │        │   pthread   │        │   pthread   │
      └──────┬──────┘        └──────┬──────┘        └──────┬──────┘
             │                      │                      │
             └──────────────────────┼──────────────────────┘
                                    │
                                    ▼
                    ┌─────────────────────────────────┐
                    │        HTTP HANDLER              │
                    │                                 │
                    │  1. Receive HTTP Request        │
                    │  2. Check Request Size          │
                    │  3. Parse HTTP Request           │
                    │  4. Validate HTTP Method         │
                    │  5. Validate Request Path        │
                    │  6. Select Requested Resource    │
                    └───────────────┬─────────────────┘
                                    │
                 ┌──────────────────┼──────────────────┐
                 │                  │                  │
                 ▼                  ▼                  ▼
      ┌──────────────────┐  ┌───────────────┐  ┌──────────────────┐
      │  HTTP PARSER     │  │   SECURITY    │  │ ROUTE HANDLER    │
      │                  │  │    LAYER      │  │                  │
      │ Method           │  │               │  │ /status          │
      │ Path             │  │ Path Safety   │  │ Static Files     │
      │ HTTP Version     │  │ Traversal     │  │ MIME Detection   │
      │                  │  │ Protection    │  │                  │
      │ HTTP/1.0         │  │ Control Chars │  │ index.html       │
      │ HTTP/1.1         │  │ Backslash     │  │ CSS / JS / Images│
      └──────────────────┘  │ Request Size  │  └────────┬─────────┘
                            └───────────────┘           │
                                                       ▼
                                            ┌─────────────────────┐
                                            │    FILE SYSTEM      │
                                            │                     │
                                            │       www/          │
                                            │                     │
                                            │  index.html         │
                                            │  style.css          │
                                            │  script.js          │
                                            │  404.html            │
                                            └──────────┬──────────┘
                                                       │
                                                       ▼
                                            ┌─────────────────────┐
                                            │   HTTP RESPONSE     │
                                            │                     │
                                            │ HTTP/1.1 200 OK     │
                                            │ HTTP/1.1 400 Error  │
                                            │ HTTP/1.1 404 NotFound│
                                            │ HTTP/1.1 405 Error  │
                                            │ HTTP/1.1 413 Error  │
                                            │ HTTP/1.1 500 Error  │
                                            └──────────┬──────────┘
                                                       │
                                                       ▼
                                                 HTTP CLIENT


══════════════════════════════════════════════════════════════════════

                    OPERATING SYSTEM SERVICES

 ┌────────────────┐   ┌────────────────┐   ┌────────────────────────┐
 │   THREADING    │   │      IPC       │   │    SIGNAL HANDLING     │
 │                │   │                │   │                        │
 │ pthread_create │   │ pipe()         │   │ SIGINT                 │
 │ pthread_join   │   │ fork()         │   │ SIGTERM                │
 │ Mutex          │   │ IPC Events     │   │                        │
 │ Conditions     │   │ Monitor Process│   │ Graceful Shutdown      │
 └────────────────┘   └───────┬────────┘   └────────────────────────┘
                              │
                              ▼
                    ┌────────────────────┐
                    │   IPC MONITOR      │
                    │   CHILD PROCESS     │
                    │                    │
                    │ Connection Events  │
                    │ Request Events     │
                    │ Shutdown Events    │
                    └────────────────────┘


══════════════════════════════════════════════════════════════════════

                     MONITORING & LOGGING

 ┌──────────────────────────────────────────────────────────────────┐
 │                         MONITORING                               │
 │                                                                  │
 │ Total Connections    │ Total Requests    │ Successful Requests   │
 │ Failed Requests      │ Active Workers    │ Queue Size            │
 │ Server Uptime        │ /status Web Page                         │
 └──────────────────────────────────────────────────────────────────┘

 ┌──────────────────────────────────────────────────────────────────┐
 │                           LOGGING                                │
 │                                                                  │
 │ Timestamp | Worker ID | Client IP | Port | Method | Path        │
 │ HTTP Status Code | Response Size                              │
 └──────────────────────────────────────────────────────────────────┘


══════════════════════════════════════════════════════════════════════

                         COMPLETE FLOW

 CLIENT
   ↓
 TCP SOCKET
   ↓
 accept()
   ↓
 REQUEST QUEUE
   ↓
 WORKER THREAD
   ↓
 HTTP PARSER
   ↓
 SECURITY VALIDATION
   ↓
 ROUTE / STATIC FILE
   ↓
 HTTP RESPONSE
   ↓
 CLIENT

              + IPC → Monitoring Process
              + Logging → Request Log
              + Monitoring → Runtime Statistics
              + Signals → Graceful Shutdown


KEY OS CONCEPTS DEMONSTRATED

✓ TCP/IP Networking
✓ Processes and fork()
✓ POSIX Threads
✓ Producer–Consumer Problem
✓ Mutex Synchronization
✓ Condition Variables
✓ Inter-Process Communication
✓ Signals
✓ File Descriptors
✓ File-System Operations
✓ Resource Management
✓ Security Validation
✓ Logging and Monitoring
✓ Concurrent Request Processing
