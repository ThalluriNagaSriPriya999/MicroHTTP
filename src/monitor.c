#define _POSIX_C_SOURCE 200809L

#include "monitor.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static monitor_stats_t statistics;

static pthread_mutex_t monitor_mutex;

static int monitor_initialized = 0;

static time_t start_time;

int monitor_init(void)
{
    memset(&statistics, 0, sizeof(statistics));

    start_time = time(NULL);

    if (start_time == (time_t)-1)
    {
        return -1;
    }

    if (pthread_mutex_init(&monitor_mutex, NULL) != 0)
    {
        return -1;
    }

    monitor_initialized = 1;

    return 0;
}

void monitor_record_connection(void)
{
    if (!monitor_initialized)
    {
        return;
    }

    pthread_mutex_lock(&monitor_mutex);

    statistics.total_connections++;

    pthread_mutex_unlock(&monitor_mutex);
}

void monitor_record_request(int status_code)
{
    if (!monitor_initialized)
    {
        return;
    }

    pthread_mutex_lock(&monitor_mutex);

    statistics.total_requests++;

    if (status_code >= 200 &&
        status_code < 400)
    {
        statistics.successful_requests++;
    }
    else
    {
        statistics.failed_requests++;
    }

    pthread_mutex_unlock(&monitor_mutex);
}

void monitor_worker_started(void)
{
    if (!monitor_initialized)
    {
        return;
    }

    pthread_mutex_lock(&monitor_mutex);

    statistics.active_workers++;

    pthread_mutex_unlock(&monitor_mutex);
}

void monitor_worker_stopped(void)
{
    if (!monitor_initialized)
    {
        return;
    }

    pthread_mutex_lock(&monitor_mutex);

    if (statistics.active_workers > 0)
    {
        statistics.active_workers--;
    }

    pthread_mutex_unlock(&monitor_mutex);
}

void monitor_set_queue_size(size_t queue_size)
{
    if (!monitor_initialized)
    {
        return;
    }

    pthread_mutex_lock(&monitor_mutex);

    statistics.queue_size = queue_size;

    pthread_mutex_unlock(&monitor_mutex);
}

void monitor_get_stats(monitor_stats_t *stats)
{
    if (!monitor_initialized ||
        stats == NULL)
    {
        return;
    }

    pthread_mutex_lock(&monitor_mutex);

    /*
     * Calculate the server uptime whenever
     * monitoring statistics are requested.
     */
    time_t current_time = time(NULL);

    if (current_time != (time_t)-1 &&
        current_time >= start_time)
    {
        statistics.uptime_seconds =
            (unsigned long)(current_time - start_time);
    }

    *stats = statistics;

    pthread_mutex_unlock(&monitor_mutex);
}

void monitor_print_stats(void)
{
    if (!monitor_initialized)
    {
        return;
    }

    monitor_stats_t stats;

    monitor_get_stats(&stats);

    unsigned long hours =
        stats.uptime_seconds / 3600;

    unsigned long minutes =
        (stats.uptime_seconds % 3600) / 60;

    unsigned long seconds =
        stats.uptime_seconds % 60;

    printf("\n");
    printf("========================================\n");
    printf("       MicroHTTP Monitoring Stats\n");
    printf("========================================\n");

    printf("Uptime            : %02lu:%02lu:%02lu\n",
           hours,
           minutes,
           seconds);

    printf("Total Connections : %lu\n",
           stats.total_connections);

    printf("Total Requests    : %lu\n",
           stats.total_requests);

    printf("Successful        : %lu\n",
           stats.successful_requests);

    printf("Failed            : %lu\n",
           stats.failed_requests);

    printf("Active Workers    : %lu\n",
           stats.active_workers);

    printf("Queue Size        : %zu\n",
           stats.queue_size);

    printf("========================================\n");
}

void monitor_shutdown(void)
{
    if (!monitor_initialized)
    {
        return;
    }

    pthread_mutex_destroy(&monitor_mutex);

    monitor_initialized = 0;
}
