#ifndef MONITOR_H
#define MONITOR_H

#include <stddef.h>

typedef struct
{
    unsigned long total_requests;
    unsigned long successful_requests;
    unsigned long failed_requests;
    unsigned long total_connections;
    unsigned long active_workers;
    size_t queue_size;
    unsigned long uptime_seconds;

} monitor_stats_t;

int monitor_init(void);

void monitor_record_connection(void);

void monitor_record_request(int status_code);

void monitor_worker_started(void);

void monitor_worker_stopped(void);

void monitor_set_queue_size(size_t queue_size);

void monitor_get_stats(monitor_stats_t *stats);

void monitor_print_stats(void);

void monitor_shutdown(void);

#endif
