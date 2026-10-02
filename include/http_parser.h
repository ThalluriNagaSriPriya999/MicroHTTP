#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#define HTTP_METHOD_MAX 16
#define HTTP_PATH_MAX 2048
#define HTTP_VERSION_MAX 16

#define HTTP_PARSE_OK 0
#define HTTP_PARSE_BAD_REQUEST -1

typedef struct
{
    char method[HTTP_METHOD_MAX];
    char path[HTTP_PATH_MAX];
    char version[HTTP_VERSION_MAX];

} http_request_t;

int http_parse_request(const char *raw_request,
                       http_request_t *request);

#endif
