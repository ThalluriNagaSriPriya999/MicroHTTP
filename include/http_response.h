#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>

#define HTTP_OK 200
#define HTTP_BAD_REQUEST 400
#define HTTP_NOT_FOUND 404
#define HTTP_METHOD_NOT_ALLOWED 405
#define HTTP_INTERNAL_SERVER_ERROR 500

int http_send_response(int client_fd,
                       int status_code,
                       const char *reason_phrase,
                       const char *content_type,
                       const char *body,
                       size_t body_length);

#endif
