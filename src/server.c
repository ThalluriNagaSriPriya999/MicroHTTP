#include "server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int server_create(int port)
{
    int server_fd;

    /* Create an IPv4 TCP socket */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("socket");
        return -1;
    }

    /* Allow the address to be reused after server restart */
    int opt = 1;

    if (setsockopt(server_fd,
                   SOL_SOCKET,
                   SO_REUSEADDR,
                   &opt,
                   sizeof(opt)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        return -1;
    }

    /* Configure server address */
    struct sockaddr_in server_address;

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons((uint16_t)port);

    /* Bind socket to the requested port */
    if (bind(server_fd,
             (struct sockaddr *)&server_address,
             sizeof(server_address)) == -1)
    {
        perror("bind");
        close(server_fd);
        return -1;
    }

    /* Start listening for incoming connections */
    if (listen(server_fd, BACKLOG) == -1)
    {
        perror("listen");
        close(server_fd);
        return -1;
    }

    printf("MicroHTTP listening on port %d\n", port);

    return server_fd;
}
