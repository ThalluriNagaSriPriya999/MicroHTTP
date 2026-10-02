#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "request_queue.h"

void http_handle_client(const client_connection_t *connection,
                        unsigned long worker_id);

#endif
