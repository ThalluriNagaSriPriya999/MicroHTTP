#include "http_handler.h"

#include "http_parser.h"
#include "http_response.h"
#include "ipc.h"
#include "logger.h"
#include "monitor.h"
#include "static_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
#define MAX_HTTP_REQUEST_SIZE (BUFFER_SIZE - 1)

/*
 * Send an HTTP request event to the IPC monitor.
 */
static void send_ipc_request_event(
    const client_connection_t *connection,
    unsigned long worker_id,
    const char *path,
    int status_code)
{
    if (connection == NULL ||
        path == NULL)
    {
        return;
    }

    ipc_message_t message;

    memset(&message, 0, sizeof(message));

    message.event_type = IPC_EVENT_REQUEST;
    message.status_code = status_code;
    message.worker_id = worker_id;

    snprintf(message.client_ip,
             sizeof(message.client_ip),
             "%s",
             connection->client_ip);

    snprintf(message.path,
             sizeof(message.path),
             "%s",
             path);

    if (ipc_send_message(&message) != 0)
    {
        fprintf(stderr,
                "Failed to send IPC request event.\n");
    }
}

/*
 * Send the server monitoring status page.
 */
static void send_status_page(int client_fd)
{
    monitor_stats_t stats;

    monitor_get_stats(&stats);

    unsigned long hours =
        stats.uptime_seconds / 3600;

    unsigned long minutes =
        (stats.uptime_seconds % 3600) / 60;

    unsigned long seconds =
        stats.uptime_seconds % 60;

    char status_body[4096];

    int body_length = snprintf(
        status_body,
        sizeof(status_body),

        "<!DOCTYPE html>"
        "<html lang=\"en\">"

        "<head>"
        "<meta charset=\"UTF-8\">"
        "<meta name=\"viewport\" "
        "content=\"width=device-width, initial-scale=1.0\">"
        "<title>MicroHTTP Status</title>"

        "<style>"
        "body{"
        "font-family:Arial,sans-serif;"
        "background:#f4f6f8;"
        "margin:40px;"
        "}"

        ".container{"
        "max-width:800px;"
        "margin:auto;"
        "background:white;"
        "padding:30px;"
        "border-radius:12px;"
        "box-shadow:0 4px 15px rgba(0,0,0,0.1);"
        "}"

        "h1{"
        "margin-top:0;"
        "}"

        ".status{"
        "display:inline-block;"
        "padding:8px 15px;"
        "background:#d4edda;"
        "color:#155724;"
        "border-radius:20px;"
        "font-weight:bold;"
        "}"

        "table{"
        "width:100%%;"
        "border-collapse:collapse;"
        "margin-top:20px;"
        "}"

        "th,td{"
        "padding:12px;"
        "border-bottom:1px solid #ddd;"
        "text-align:left;"
        "}"

        "th{"
        "background:#f1f1f1;"
        "}"

        ".footer{"
        "margin-top:25px;"
        "color:#666;"
        "font-size:14px;"
        "}"
        "</style>"

        "</head>"

        "<body>"

        "<div class=\"container\">"

        "<h1>MicroHTTP Server Status</h1>"

        "<div class=\"status\">SERVER ONLINE</div>"

        "<table>"

        "<tr>"
        "<th>Metric</th>"
        "<th>Value</th>"
        "</tr>"

        "<tr>"
        "<td>Uptime</td>"
        "<td>%02lu:%02lu:%02lu</td>"
        "</tr>"

        "<tr>"
        "<td>Total Connections</td>"
        "<td>%lu</td>"
        "</tr>"

        "<tr>"
        "<td>Total Requests</td>"
        "<td>%lu</td>"
        "</tr>"

        "<tr>"
        "<td>Successful Requests</td>"
        "<td>%lu</td>"
        "</tr>"

        "<tr>"
        "<td>Failed Requests</td>"
        "<td>%lu</td>"
        "</tr>"

        "<tr>"
        "<td>Active Workers</td>"
        "<td>%lu</td>"
        "</tr>"

        "<tr>"
        "<td>Queue Size</td>"
        "<td>%zu</td>"
        "</tr>"

        "</table>"

        "<div class=\"footer\">"
        "MicroHTTP - Concurrent C Web Server"
        "</div>"

        "</div>"

        "</body>"
        "</html>",

        hours,
        minutes,
        seconds,
        stats.total_connections,
        stats.total_requests,
        stats.successful_requests,
        stats.failed_requests,
        stats.active_workers,
        stats.queue_size
    );

    if (body_length < 0 ||
        (size_t)body_length >= sizeof(status_body))
    {
        const char *error_body =
            "<html>"
            "<head><title>500 Internal Server Error</title></head>"
            "<body>"
            "<h1>500 Internal Server Error</h1>"
            "</body>"
            "</html>";

        http_send_response(
            client_fd,
            HTTP_INTERNAL_SERVER_ERROR,
            "Internal Server Error",
            "text/html",
            error_body,
            strlen(error_body));

        monitor_record_request(
            HTTP_INTERNAL_SERVER_ERROR);

        return;
    }

    if (http_send_response(
            client_fd,
            HTTP_OK,
            "OK",
            "text/html",
            status_body,
            (size_t)body_length) == -1)
    {
        logger_log_error(
            "Failed to send /status response.");

        monitor_record_request(
            HTTP_INTERNAL_SERVER_ERROR);

        return;
    }

    monitor_record_request(HTTP_OK);
}

void http_handle_client(const client_connection_t *connection,
                        unsigned long worker_id)
{
    if (connection == NULL ||
        connection->client_fd < 0)
    {
        return;
    }

    int client_fd = connection->client_fd;

    char request_buffer[BUFFER_SIZE];

    ssize_t bytes_received =
    recv(client_fd,
         request_buffer,
         sizeof(request_buffer) - 1,
         0);

if (bytes_received <= 0)
{
    logger_log_error(
        "Failed to receive HTTP request.");

    close(client_fd);
    return;
}

if ((size_t)bytes_received >= MAX_HTTP_REQUEST_SIZE)
{
    const char *error_body =
        "<html>"
        "<head><title>413 Request Too Large</title></head>"
        "<body>"
        "<h1>413 Request Too Large</h1>"
        "<p>The HTTP request exceeds the maximum supported size.</p>"
        "</body>"
        "</html>";

    http_send_response(
        client_fd,
        413,
        "Request Too Large",
        "text/html",
        error_body,
        strlen(error_body));

    monitor_record_request(413);

    logger_log_request(
        connection->client_ip,
        connection->client_port,
        worker_id,
        "UNKNOWN",
        "UNKNOWN",
        413,
        strlen(error_body));

    send_ipc_request_event(
        connection,
        worker_id,
        "UNKNOWN",
        413);

    close(client_fd);
    return;
}

request_buffer[bytes_received] = '\0';

    printf("\n----- HTTP Request -----\n");
    printf("%s", request_buffer);
    printf("----- End Request -----\n");

    http_request_t request;

    int parse_result =
        http_parse_request(
            request_buffer,
            &request);

    if (parse_result != HTTP_PARSE_OK)
    {
        const char *error_body =
            "<html>"
            "<head><title>400 Bad Request</title></head>"
            "<body>"
            "<h1>400 Bad Request</h1>"
            "</body>"
            "</html>";

        http_send_response(
            client_fd,
            HTTP_BAD_REQUEST,
            "Bad Request",
            "text/html",
            error_body,
            strlen(error_body));

        monitor_record_request(
            HTTP_BAD_REQUEST);

        logger_log_request(
            connection->client_ip,
            connection->client_port,
            worker_id,
            "UNKNOWN",
            "UNKNOWN",
            HTTP_BAD_REQUEST,
            strlen(error_body));

        send_ipc_request_event(
            connection,
            worker_id,
            "UNKNOWN",
            HTTP_BAD_REQUEST);

        close(client_fd);
        return;
    }

    printf("----- Parsed Request -----\n");
    printf("Method  : %s\n", request.method);
    printf("Path    : %s\n", request.path);
    printf("Version : %s\n", request.version);
    printf("----- End Parsed Request -----\n");

    /*
     * MicroHTTP currently supports GET.
     */
    if (strcmp(request.method, "GET") != 0)
    {
        const char *method_body =
            "<html>"
            "<head>"
            "<title>405 Method Not Allowed</title>"
            "</head>"
            "<body>"
            "<h1>405 Method Not Allowed</h1>"
            "<p>"
            "MicroHTTP currently supports GET requests."
            "</p>"
            "</body>"
            "</html>";

        http_send_response(
            client_fd,
            HTTP_METHOD_NOT_ALLOWED,
            "Method Not Allowed",
            "text/html",
            method_body,
            strlen(method_body));

        monitor_record_request(
            HTTP_METHOD_NOT_ALLOWED);

        logger_log_request(
            connection->client_ip,
            connection->client_port,
            worker_id,
            request.method,
            request.path,
            HTTP_METHOD_NOT_ALLOWED,
            strlen(method_body));

        send_ipc_request_event(
            connection,
            worker_id,
            request.path,
            HTTP_METHOD_NOT_ALLOWED);

        close(client_fd);
        return;
    }

    /*
     * Built-in monitoring endpoint.
     */
    if (strcmp(request.path, "/status") == 0)
    {
        send_status_page(client_fd);

        logger_log_request(
            connection->client_ip,
            connection->client_port,
            worker_id,
            request.method,
            request.path,
            HTTP_OK,
            0);

        send_ipc_request_event(
            connection,
            worker_id,
            request.path,
            HTTP_OK);

        close(client_fd);
        return;
    }

    char *file_data = NULL;

    size_t file_size = 0;

    const char *content_type = NULL;

    int file_result =
        static_file_read(
            request.path,
            &file_data,
            &file_size,
            &content_type);

    if (file_result != 0)
    {
        const char *not_found_body =
            "<html>"
            "<head>"
            "<title>404 Not Found</title>"
            "</head>"
            "<body>"
            "<h1>404 Not Found</h1>"
            "<p>"
            "The requested resource was not found."
            "</p>"
            "</body>"
            "</html>";

        http_send_response(
            client_fd,
            HTTP_NOT_FOUND,
            "Not Found",
            "text/html",
            not_found_body,
            strlen(not_found_body));

        monitor_record_request(
            HTTP_NOT_FOUND);

        logger_log_request(
            connection->client_ip,
            connection->client_port,
            worker_id,
            request.method,
            request.path,
            HTTP_NOT_FOUND,
            strlen(not_found_body));

        send_ipc_request_event(
            connection,
            worker_id,
            request.path,
            HTTP_NOT_FOUND);

        close(client_fd);
        return;
    }

    printf("Static file found.\n");

    printf("Content-Type : %s\n",
           content_type);

    printf("File Size    : %zu bytes\n",
           file_size);

    if (http_send_response(
            client_fd,
            HTTP_OK,
            "OK",
            content_type,
            file_data,
            file_size) == -1)
    {
        fprintf(stderr,
                "Failed to send HTTP response.\n");

        logger_log_error(
            "Failed to send HTTP response.");

        monitor_record_request(
            HTTP_INTERNAL_SERVER_ERROR);

        logger_log_request(
            connection->client_ip,
            connection->client_port,
            worker_id,
            request.method,
            request.path,
            HTTP_INTERNAL_SERVER_ERROR,
            0);

        send_ipc_request_event(
            connection,
            worker_id,
            request.path,
            HTTP_INTERNAL_SERVER_ERROR);
    }
    else
    {
        monitor_record_request(
            HTTP_OK);

        logger_log_request(
            connection->client_ip,
            connection->client_port,
            worker_id,
            request.method,
            request.path,
            HTTP_OK,
            file_size);

        send_ipc_request_event(
            connection,
            worker_id,
            request.path,
            HTTP_OK);
    }

    free(file_data);

    close(client_fd);
}
