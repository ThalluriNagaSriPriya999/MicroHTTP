#include "http_parser.h"

#include <stdio.h>
#include <string.h>

int http_parse_request(const char *raw_request,
                       http_request_t *request)
{
    if (raw_request == NULL || request == NULL)
    {
        return HTTP_PARSE_BAD_REQUEST;
    }

    /*
     * Extract the HTTP method, request path,
     * and HTTP version from the first request line.
     */
    int fields = sscanf(raw_request,
                        "%15s %2047s %15s",
                        request->method,
                        request->path,
                        request->version);

    if (fields != 3)
    {
        return HTTP_PARSE_BAD_REQUEST;
    }

    /*
     * Currently MicroHTTP supports HTTP/1.0
     * and HTTP/1.1 requests.
     */
    if (strcmp(request->version, "HTTP/1.0") != 0 &&
        strcmp(request->version, "HTTP/1.1") != 0)
    {
        return HTTP_PARSE_BAD_REQUEST;
    }

    return HTTP_PARSE_OK;
}
