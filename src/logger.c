#define _POSIX_C_SOURCE 200809L
#include "logger.h"

#include <pthread.h>
#include <stdio.h>
#include <time.h>

static FILE *log_file = NULL;

static pthread_mutex_t log_mutex;

static int logger_initialized = 0;

static void get_timestamp(char *buffer,
                          size_t buffer_size)
{
    time_t current_time = time(NULL);

    struct tm time_info;

    if (localtime_r(&current_time, &time_info) == NULL)
    {
        snprintf(buffer,
                 buffer_size,
                 "UNKNOWN-TIME");

        return;
    }

    strftime(buffer,
             buffer_size,
             "%Y-%m-%d %H:%M:%S",
             &time_info);
}

int logger_init(const char *log_file_path)
{
    if (log_file_path == NULL)
    {
        return -1;
    }

    log_file = fopen(log_file_path, "a");

    if (log_file == NULL)
    {
        perror("fopen log file");
        return -1;
    }

    if (pthread_mutex_init(&log_mutex, NULL) != 0)
    {
        fclose(log_file);
        log_file = NULL;

        return -1;
    }

    logger_initialized = 1;

    return 0;
}

void logger_log_request(const char *client_ip,
                        int client_port,
                        unsigned long worker_id,
                        const char *method,
                        const char *path,
                        int status_code,
                        size_t response_size)
{
    if (!logger_initialized ||
        log_file == NULL)
    {
        return;
    }

    char timestamp[32];

    get_timestamp(timestamp,
                  sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    fprintf(log_file,
            "[%s] "
            "[WORKER:%lu] "
            "[CLIENT:%s:%d] "
            "%s %s "
            "-> %d "
            "(%zu bytes)\n",
            timestamp,
            worker_id,
            client_ip,
            client_port,
            method,
            path,
            status_code,
            response_size);

    fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

void logger_log_error(const char *message)
{
    if (!logger_initialized ||
        log_file == NULL ||
        message == NULL)
    {
        return;
    }

    char timestamp[32];

    get_timestamp(timestamp,
                  sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    fprintf(log_file,
            "[%s] [ERROR] %s\n",
            timestamp,
            message);

    fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

void logger_shutdown(void)
{
    if (!logger_initialized)
    {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    if (log_file != NULL)
    {
        fclose(log_file);
        log_file = NULL;
    }

    pthread_mutex_unlock(&log_mutex);

    pthread_mutex_destroy(&log_mutex);

    logger_initialized = 0;
}
