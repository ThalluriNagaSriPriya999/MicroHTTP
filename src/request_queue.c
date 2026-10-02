#include "request_queue.h"

#include "monitor.h"

int request_queue_init(request_queue_t *queue)
{
    if (queue == NULL)
    {
        return -1;
    }

    queue->front = 0;
    queue->rear = 0;
    queue->count = 0;
    queue->shutdown = 0;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0)
    {
        return -1;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0)
    {
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0)
    {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }

    /*
     * The queue starts empty.
     */
    monitor_set_queue_size(0);

    return 0;
}

int request_queue_push(request_queue_t *queue,
                       const client_connection_t *connection)
{
    if (queue == NULL || connection == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    /*
     * Wait until space becomes available.
     */
    while (request_queue_is_full(queue) &&
           !queue->shutdown)
    {
        pthread_cond_wait(&queue->not_full,
                          &queue->mutex);
    }

    /*
     * Server is shutting down.
     */
    if (queue->shutdown)
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /*
     * Insert the connection at the rear.
     */
    queue->connections[queue->rear] = *connection;

    queue->rear =
        (queue->rear + 1) % REQUEST_QUEUE_CAPACITY;

    queue->count++;

    /*
     * Update monitoring immediately after
     * the queue size changes.
     */
    monitor_set_queue_size(
        (size_t)queue->count);

    /*
     * Wake one worker waiting for work.
     */
    pthread_cond_signal(&queue->not_empty);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

int request_queue_pop(request_queue_t *queue,
                      client_connection_t *connection)
{
    if (queue == NULL || connection == NULL)
    {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    /*
     * Wait until a connection becomes available.
     */
    while (request_queue_is_empty(queue) &&
           !queue->shutdown)
    {
        pthread_cond_wait(&queue->not_empty,
                          &queue->mutex);
    }

    /*
     * During shutdown, return when there is
     * no remaining work.
     */
    if (queue->shutdown &&
        request_queue_is_empty(queue))
    {
        pthread_mutex_unlock(&queue->mutex);
        return -1;
    }

    /*
     * Remove the connection from the front.
     */
    *connection = queue->connections[queue->front];

    queue->front =
        (queue->front + 1) % REQUEST_QUEUE_CAPACITY;

    queue->count--;

    /*
     * Update monitoring immediately after
     * removing the connection.
     */
    monitor_set_queue_size(
        (size_t)queue->count);

    /*
     * Wake one producer waiting for space.
     */
    pthread_cond_signal(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

void request_queue_shutdown(request_queue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }

    pthread_mutex_lock(&queue->mutex);

    queue->shutdown = 1;

    /*
     * Wake all waiting workers and producers.
     */
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);

    pthread_mutex_unlock(&queue->mutex);
}

void request_queue_destroy(request_queue_t *queue)
{
    if (queue == NULL)
    {
        return;
    }

    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->mutex);
}

int request_queue_is_empty(const request_queue_t *queue)
{
    if (queue == NULL)
    {
        return 1;
    }

    return queue->count == 0;
}

int request_queue_is_full(const request_queue_t *queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    return queue->count == REQUEST_QUEUE_CAPACITY;
}

size_t request_queue_size(request_queue_t *queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);

    size_t size = (size_t)queue->count;

    pthread_mutex_unlock(&queue->mutex);

    return size;
}
