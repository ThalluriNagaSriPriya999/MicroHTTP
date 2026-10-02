#include "http_parser.h"

#include <stdio.h>
#include <string.h>

static int tests_passed = 0;
static int tests_failed = 0;

static void check_test(const char *name, int condition)
{
    if (condition)
    {
        printf("[PASS] %s\n", name);
        tests_passed++;
    }
    else
    {
        printf("[FAIL] %s\n", name);
        tests_failed++;
    }
}

int main(void)
{
    http_request_t request;

    int result = http_parse_request(
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n",
        &request);

    check_test(
        "Valid GET request",
        result == HTTP_PARSE_OK &&
        strcmp(request.method, "GET") == 0 &&
        strcmp(request.path, "/") == 0 &&
        strcmp(request.version, "HTTP/1.1") == 0);

    result = http_parse_request(
        "GET /index.html HTTP/1.0\r\n"
        "\r\n",
        &request);

    check_test(
        "Valid HTTP/1.0 request",
        result == HTTP_PARSE_OK &&
        strcmp(request.path, "/index.html") == 0);

    result = http_parse_request(
        "INVALID REQUEST",
        &request);

    check_test(
        "Invalid request rejected",
        result == HTTP_PARSE_BAD_REQUEST);

    result = http_parse_request(
        "GET / HTTP/2.0\r\n"
        "\r\n",
        &request);

    check_test(
        "Unsupported HTTP version rejected",
        result == HTTP_PARSE_BAD_REQUEST);

    result = http_parse_request(
        NULL,
        &request);

    check_test(
        "NULL request rejected",
        result == HTTP_PARSE_BAD_REQUEST);

    printf("\n========================================\n");
    printf("HTTP Parser Test Results\n");
    printf("========================================\n");
    printf("Passed : %d\n", tests_passed);
    printf("Failed : %d\n", tests_failed);
    printf("========================================\n");

    return tests_failed == 0 ? 0 : 1;
}
