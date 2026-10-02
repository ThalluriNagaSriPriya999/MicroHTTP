#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>

int logger_init(const char *log_file);

void logger_log_request(const char *client_ip,
                        int client_port,
                        unsigned long worker_id,
                        const char *method,
                        const char *path,
                        int status_code,
                        size_t response_size);

void logger_log_error(const char *message);

void logger_shutdown(void);

#endif
