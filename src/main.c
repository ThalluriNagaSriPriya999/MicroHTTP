#define _POSIX_C_SOURCE 200809L

#include "http_handler.h"
#include "ipc.h"
#include "logger.h"
#include "monitor.h"
#include "request_queue.h"
#include "server.h"
#include "signal_handler.h"
#include "thread_pool.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_WORKER_COUNT 4
#define CLIENT_IP_MAX_LENGTH 64

static void print_usage(const char *program_name)
{
    printf("Usage: %s [port] [worker_count]\n",
           program_name);

    printf("Example: %s 8081 4\n",
           program_name);
}

int main(int argc, char *argv[])
{
    int port = 8081;
    int worker_count = DEFAULT_WORKER_COUNT;

    if (argc >= 2)
    {
        char *end_pointer = NULL;

        long parsed_port =
            strtol(argv[1], &end_pointer, 10);

        if (*argv[1] == '\0' ||
            end_pointer == NULL ||
            *end_pointer != '\0' ||
            parsed_port < 1 ||
            parsed_port > 65535)
        {
            fprintf(stderr,
                    "Invalid port number.\n");

            print_usage(argv[0]);

            return EXIT_FAILURE;
        }

        port = (int)parsed_port;
    }

    if (argc >= 3)
    {
        char *end_pointer = NULL;

        long parsed_workers =
            strtol(argv[2], &end_pointer, 10);

        if (*argv[2] == '\0' ||
            end_pointer == NULL ||
            *end_pointer != '\0' ||
            parsed_workers <= 0 ||
            parsed_workers > 128)
        {
            fprintf(stderr,
                    "Invalid worker count.\n");

            print_usage(argv[0]);

            return EXIT_FAILURE;
        }

        worker_count = (int)parsed_workers;
    }

    if (argc > 3)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("\n");
    printf("========================================\n");
    printf("          MicroHTTP Server\n");
    printf("========================================\n");
    printf("Port          : %d\n", port);
    printf("Worker Threads: %d\n", worker_count);
    printf("========================================\n");
    printf("\n");

    /*
     * Initialize signal handling first.
     *
     * SIGINT  -> Ctrl+C
     * SIGTERM -> termination request
     */
    if (signal_handler_init() != 0)
    {
        fprintf(stderr,
                "Failed to initialize signal handling.\n");

        return EXIT_FAILURE;
    }

    /*
     * Initialize monitoring.
     */
    if (monitor_init() != 0)
    {
        fprintf(stderr,
                "Failed to initialize monitoring.\n");

        return EXIT_FAILURE;
    }

    /*
     * Initialize logger.
     */
    if (logger_init("logs/microhttp.log") != 0)
    {
        fprintf(stderr,
                "Failed to initialize logger.\n");

        monitor_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Create IPC pipe.
     */
    if (ipc_create() != 0)
    {
        fprintf(stderr,
                "Failed to create IPC.\n");

        logger_shutdown();
        monitor_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Start IPC monitor process.
     */
    if (ipc_start_monitor() != 0)
    {
        fprintf(stderr,
                "Failed to start IPC monitor.\n");

        ipc_shutdown();
        logger_shutdown();
        monitor_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Create request queue.
     */
    request_queue_t request_queue;

    if (request_queue_init(&request_queue) != 0)
    {
        fprintf(stderr,
                "Failed to initialize request queue.\n");

        ipc_shutdown();
        logger_shutdown();
        monitor_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Create worker thread pool.
     */
    thread_pool_t thread_pool;

    if (thread_pool_init(&thread_pool,
                         worker_count,
                         &request_queue) != 0)
    {
        fprintf(stderr,
                "Failed to initialize thread pool.\n");

        request_queue_destroy(&request_queue);
        ipc_shutdown();
        logger_shutdown();
        monitor_shutdown();

        return EXIT_FAILURE;
    }

    /*
     * Create and configure TCP server socket.
     */
    int server_fd = server_create(port);

    if (server_fd < 0)
    {
        fprintf(stderr,
                "Failed to create server socket.\n");

        thread_pool_destroy(&thread_pool);
        request_queue_destroy(&request_queue);
        ipc_shutdown();
        logger_shutdown();
        monitor_shutdown();

        return EXIT_FAILURE;
    }

    printf("\n");
    printf("========================================\n");
    printf("MicroHTTP is ready.\n");
    printf("Open: http://127.0.0.1:%d/\n", port);
    printf("Press Ctrl+C to stop the server.\n");
    printf("========================================\n");
    printf("\n");

    /*
     * Main server loop.
     *
     * The server continues accepting clients until
     * SIGINT or SIGTERM sets the shutdown flag.
     */
    while (!signal_handler_shutdown_requested())
    {
        struct sockaddr_in client_address;

        socklen_t client_address_length =
            sizeof(client_address);

        int client_fd =
            accept(server_fd,
                   (struct sockaddr *)&client_address,
                   &client_address_length);

        if (client_fd < 0)
        {
            if (errno == EINTR)
            {
                /*
                 * A signal interrupted accept().
                 * Check the shutdown flag again.
                 */
                continue;
            }

            perror("accept");

            if (signal_handler_shutdown_requested())
            {
                break;
            }

            continue;
        }

        /*
         * If shutdown was requested immediately after
         * accept(), close the newly accepted connection
         * instead of placing it into the queue.
         */
        if (signal_handler_shutdown_requested())
        {
            close(client_fd);
            break;
        }

        char client_ip[CLIENT_IP_MAX_LENGTH];

        memset(client_ip,
               0,
               sizeof(client_ip));

        const char *ip_result =
            inet_ntop(AF_INET,
                      &client_address.sin_addr,
                      client_ip,
                      sizeof(client_ip));

        if (ip_result == NULL)
        {
            snprintf(client_ip,
                     sizeof(client_ip),
                     "UNKNOWN");
        }

        int client_port =
            ntohs(client_address.sin_port);

        client_connection_t connection;

        memset(&connection,
               0,
               sizeof(connection));

        connection.client_fd = client_fd;

        memcpy(connection.client_ip,
       client_ip,
       sizeof(connection.client_ip));

connection.client_ip[sizeof(connection.client_ip) - 1] = '\0';

        connection.client_port = client_port;

        /*
         * Record the incoming connection.
         */
        monitor_record_connection();

        printf("Accepted connection from %s:%d "
               "(FD %d)\n",
               connection.client_ip,
               connection.client_port,
               connection.client_fd);

        /*
         * Send connection event through IPC.
         */
        ipc_message_t connection_message;

        memset(&connection_message,
               0,
               sizeof(connection_message));

        connection_message.event_type =
            IPC_EVENT_CONNECTION;

        snprintf(connection_message.client_ip,
                 sizeof(connection_message.client_ip),
                 "%s",
                 connection.client_ip);

        if (ipc_send_message(&connection_message) != 0)
        {
            fprintf(stderr,
                    "Failed to send IPC connection event.\n");
        }

        /*
         * Add client to producer-consumer queue.
         */
        if (request_queue_push(&request_queue,
                               &connection) != 0)
        {
            fprintf(stderr,
                    "Failed to add client to request queue.\n");

            close(client_fd);
            continue;
        }

        printf("Client queued successfully.\n");
    }

    /*
     * Graceful shutdown begins here.
     */
    printf("\n");
    printf("========================================\n");
    printf("       Shutdown Signal Received\n");
    printf("========================================\n");

    printf("Stopping new client connections...\n");

    /*
     * Close the listening socket so no new clients
     * can be accepted.
     */
    close(server_fd);

    /*
     * Wake worker threads and tell them that no more
     * requests will be accepted.
     */
    printf("Stopping request queue...\n");

    request_queue_shutdown(&request_queue);

    /*
     * Wait for all worker threads to finish.
     */
    printf("Stopping worker threads...\n");

    thread_pool_destroy(&thread_pool);

    printf("Worker threads stopped.\n");

    /*
     * Display final server statistics.
     */
    monitor_print_stats();

    /*
     * Destroy request queue synchronization objects.
     */
    request_queue_destroy(&request_queue);

    printf("Request queue destroyed.\n");

    /*
     * Shutdown logger.
     */
    logger_shutdown();

    printf("Logger shut down.\n");

    /*
     * Shutdown monitoring.
     */
    monitor_shutdown();

    printf("Monitoring shut down.\n");

    /*
     * Send IPC shutdown event and wait for
     * the monitor process.
     */
    ipc_shutdown();

    printf("MicroHTTP IPC shut down.\n");

    printf("\n");
    printf("========================================\n");
    printf("MicroHTTP shutdown complete.\n");
    printf("========================================\n");

    return EXIT_SUCCESS;
}
