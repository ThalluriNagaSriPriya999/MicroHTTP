#include "http_response.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/*
 * Send exactly the requested number of bytes.
 *
 * TCP does not guarantee that one send() call
 * transmits the entire buffer.
 */
static int send_all(int client_fd,
                    const char *data,
                    size_t data_length)
{
    size_t total_sent = 0;

    while (total_sent < data_length)
    {
        ssize_t bytes_sent =
            send(client_fd,
                 data + total_sent,
                 data_length - total_sent,
                 0);

        if (bytes_sent < 0)
        {
            /*
             * A signal may interrupt send().
             * Retry instead of treating it as a
             * connection failure.
             */
            if (errno == EINTR)
            {
                continue;
            }

            perror("send");
            return -1;
        }

        /*
         * send() returning zero means that no progress
         * was made. Avoid an infinite loop.
         */
        if (bytes_sent == 0)
        {
            fprintf(stderr,
                    "send returned 0 bytes.\n");

            return -1;
        }

        total_sent += (size_t)bytes_sent;
    }

    return 0;
}

int http_send_response(int client_fd,
                       int status_code,
                       const char *reason_phrase,
                       const char *content_type,
                       const char *body,
                       size_t body_length)
{
    if (client_fd < 0 ||
        reason_phrase == NULL ||
        content_type == NULL ||
        body == NULL)
    {
        return -1;
    }

    char header[1024];

    int header_length = snprintf(
        header,
        sizeof(header),

        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",

        status_code,
        reason_phrase,
        content_type,
        body_length);

    if (header_length < 0 ||
        (size_t)header_length >= sizeof(header))
    {
        fprintf(stderr,
                "HTTP response header is too large.\n");

        return -1;
    }

    /*
     * Send the complete HTTP header.
     */
    if (send_all(client_fd,
                 header,
                 (size_t)header_length) != 0)
    {
        return -1;
    }

    /*
     * Send the complete HTTP response body.
     */
    if (send_all(client_fd,
                 body,
                 body_length) != 0)
    {
        return -1;
    }

    return 0;
}
