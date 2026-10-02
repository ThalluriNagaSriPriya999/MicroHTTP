#define _POSIX_C_SOURCE 200809L

#include "signal_handler.h"

#include <signal.h>
#include <stdio.h>

static volatile sig_atomic_t shutdown_requested = 0;

static void handle_shutdown_signal(int signal_number)
{
    if (signal_number == SIGINT ||
        signal_number == SIGTERM)
    {
        shutdown_requested = 1;
    }
}

int signal_handler_init(void)
{
    struct sigaction action;

    action.sa_handler = handle_shutdown_signal;

    if (sigemptyset(&action.sa_mask) == -1)
    {
        perror("sigemptyset");
        return -1;
    }

    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1)
    {
        perror("sigaction SIGINT");
        return -1;
    }

    if (sigaction(SIGTERM, &action, NULL) == -1)
    {
        perror("sigaction SIGTERM");
        return -1;
    }

    shutdown_requested = 0;

    printf("Signal handling initialized.\n");
    printf("SIGINT and SIGTERM are supported.\n");

    return 0;
}

int signal_handler_shutdown_requested(void)
{
    return shutdown_requested != 0;
}

void signal_handler_reset(void)
{
    shutdown_requested = 0;
}
