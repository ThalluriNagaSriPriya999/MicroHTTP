#include "thread_pool.h"
#include "http_handler.h"
#include "monitor.h"

#include <stdio.h>
#include <stdlib.h>

static void *worker_thread(void *argument)
{
    thread_pool_t *pool = argument;

    unsigned long worker_id =
        (unsigned long)pthread_self();

    monitor_worker_started();

    printf("Worker thread %lu started.\n",
           worker_id);

    while (1)
    {
        client_connection_t connection;

        if (request_queue_pop(pool->queue,
                              &connection) != 0)
        {
            printf("Worker thread %lu shutting down.\n",
                   worker_id);

            break;
        }

        printf("Worker %lu received client FD %d "
               "from %s:%d\n",
               worker_id,
               connection.client_fd,
               connection.client_ip,
               connection.client_port);

        http_handle_client(&connection,
                           worker_id);

        printf("Worker %lu finished client FD %d\n",
               worker_id,
               connection.client_fd);
    }

    monitor_worker_stopped();

    return NULL;
}

int thread_pool_init(thread_pool_t *pool,
                     int thread_count,
                     request_queue_t *queue)
{
    if (pool == NULL ||
        thread_count <= 0 ||
        queue == NULL)
    {
        return -1;
    }

    pool->thread_count = thread_count;
    pool->queue = queue;

    pool->threads =
        malloc((size_t)thread_count * sizeof(pthread_t));

    if (pool->threads == NULL)
    {
        return -1;
    }

    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&pool->threads[i],
                           NULL,
                           worker_thread,
                           pool) != 0)
        {
            fprintf(stderr,
                    "Failed to create worker thread %d.\n",
                    i);

            request_queue_shutdown(queue);

            for (int j = 0; j < i; j++)
            {
                pthread_join(pool->threads[j], NULL);
            }

            free(pool->threads);
            pool->threads = NULL;

            return -1;
        }
    }

    return 0;
}

void thread_pool_destroy(thread_pool_t *pool)
{
    if (pool == NULL)
    {
        return;
    }

    if (pool->queue != NULL)
    {
        request_queue_shutdown(pool->queue);
    }

    if (pool->threads != NULL)
    {
        for (int i = 0; i < pool->thread_count; i++)
        {
            pthread_join(pool->threads[i], NULL);
        }
    }

    free(pool->threads);

    pool->threads = NULL;
    pool->thread_count = 0;
    pool->queue = NULL;
}
