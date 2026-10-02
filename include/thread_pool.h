#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

#include "request_queue.h"

typedef struct
{
    pthread_t *threads;
    int thread_count;

    request_queue_t *queue;

} thread_pool_t;

/*
 * Initialize and start worker threads.
 */
int thread_pool_init(thread_pool_t *pool,
                     int thread_count,
                     request_queue_t *queue);

/*
 * Destroy the thread pool.
 */
void thread_pool_destroy(thread_pool_t *pool);

#endif
