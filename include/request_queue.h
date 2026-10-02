#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>

#define REQUEST_QUEUE_CAPACITY 64
#define CLIENT_IP_MAX INET_ADDRSTRLEN

typedef struct
{
    int client_fd;
    char client_ip[CLIENT_IP_MAX];
    int client_port;

} client_connection_t;

typedef struct
{
    client_connection_t connections[REQUEST_QUEUE_CAPACITY];

    int front;
    int rear;
    int count;

    int shutdown;

    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;

} request_queue_t;

int request_queue_init(request_queue_t *queue);

int request_queue_push(request_queue_t *queue,
                       const client_connection_t *connection);

int request_queue_pop(request_queue_t *queue,
                      client_connection_t *connection);

void request_queue_shutdown(request_queue_t *queue);

void request_queue_destroy(request_queue_t *queue);

int request_queue_is_empty(const request_queue_t *queue);

int request_queue_is_full(const request_queue_t *queue);

/*
 * Return the current number of connections
 * waiting in the request queue.
 */
size_t request_queue_size(request_queue_t *queue);

#endif
