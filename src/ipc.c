#define _POSIX_C_SOURCE 200809L

#include "ipc.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int ipc_pipe[2] = {-1, -1};
static pid_t monitor_pid = -1;

/*
 * Create the pipe used for parent-child IPC.
 */
int ipc_create(void)
{
    if (pipe(ipc_pipe) == -1)
    {
        perror("pipe");
        return -1;
    }

    printf("IPC pipe created successfully.\n");

    return 0;
}

/*
 * Return the write end of the pipe.
 */
int ipc_get_write_fd(void)
{
    return ipc_pipe[1];
}

/*
 * Return the read end of the pipe.
 */
int ipc_get_read_fd(void)
{
    return ipc_pipe[0];
}

/*
 * Send one complete IPC message.
 */
int ipc_send_message(const ipc_message_t *message)
{
    if (message == NULL ||
        ipc_pipe[1] == -1)
    {
        return -1;
    }

    const char *data =
        (const char *)message;

    size_t total_written = 0;

    while (total_written < sizeof(ipc_message_t))
    {
        ssize_t bytes_written =
            write(ipc_pipe[1],
                  data + total_written,
                  sizeof(ipc_message_t) - total_written);

        if (bytes_written < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("IPC write");
            return -1;
        }

        if (bytes_written == 0)
        {
            return -1;
        }

        total_written += (size_t)bytes_written;
    }

    return 0;
}

/*
 * Monitor process.
 *
 * The child waits for messages from the
 * MicroHTTP parent process.
 */
static void monitor_process(void)
{
    /*
     * The child only reads from the pipe.
     */
    close(ipc_pipe[1]);
    ipc_pipe[1] = -1;

    printf("[IPC Monitor] Monitor process started. PID=%ld\n",
           (long)getpid());

    while (1)
    {
        ipc_message_t message;

        char *data =
            (char *)&message;

        size_t total_read = 0;

        while (total_read < sizeof(ipc_message_t))
        {
            ssize_t bytes_read =
                read(ipc_pipe[0],
                     data + total_read,
                     sizeof(ipc_message_t) - total_read);

            if (bytes_read < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }

                perror("[IPC Monitor] read");
                close(ipc_pipe[0]);
                _exit(EXIT_FAILURE);
            }

            /*
             * EOF means the parent closed the
             * write side of the pipe.
             */
            if (bytes_read == 0)
            {
                printf("[IPC Monitor] Parent closed IPC pipe.\n");
                close(ipc_pipe[0]);
                _exit(EXIT_SUCCESS);
            }

            total_read += (size_t)bytes_read;
        }

        if (message.event_type == IPC_EVENT_CONNECTION)
        {
            printf(
                "[IPC Monitor] CONNECTION "
                "from %s\n",
                message.client_ip);
        }
        else if (message.event_type == IPC_EVENT_REQUEST)
        {
            printf(
                "[IPC Monitor] REQUEST "
                "%s -> HTTP %d "
                "(Worker %lu)\n",
                message.path,
                message.status_code,
                message.worker_id);
        }
        else if (message.event_type == IPC_EVENT_SHUTDOWN)
        {
            printf(
                "[IPC Monitor] SHUTDOWN event received.\n");

            close(ipc_pipe[0]);

            _exit(EXIT_SUCCESS);
        }
        else
        {
            printf(
                "[IPC Monitor] Unknown event type: %d\n",
                message.event_type);
        }

        fflush(stdout);
    }
}

/*
 * Create the monitor child process.
 */
int ipc_start_monitor(void)
{
    if (ipc_pipe[0] == -1 ||
        ipc_pipe[1] == -1)
    {
        fprintf(stderr,
                "IPC pipe has not been created.\n");

        return -1;
    }

    monitor_pid = fork();

    if (monitor_pid == -1)
    {
        perror("fork");
        return -1;
    }

    /*
     * Child process.
     */
    if (monitor_pid == 0)
    {
        monitor_process();
        _exit(EXIT_FAILURE);
    }

    /*
     * Parent process only writes to the pipe.
     */
    close(ipc_pipe[0]);
    ipc_pipe[0] = -1;

    printf("IPC monitor process started. PID=%ld\n",
           (long)monitor_pid);

    return 0;
}

/*
 * Shut down IPC cleanly.
 */
void ipc_shutdown(void)
{
    /*
     * Send shutdown event before closing
     * the write end of the pipe.
     */
    if (ipc_pipe[1] != -1)
    {
        ipc_message_t shutdown_message;

        memset(&shutdown_message,
               0,
               sizeof(shutdown_message));

        shutdown_message.event_type =
            IPC_EVENT_SHUTDOWN;

        ipc_send_message(&shutdown_message);

        close(ipc_pipe[1]);
        ipc_pipe[1] = -1;
    }

    /*
     * Wait for the monitor child.
     */
    if (monitor_pid > 0)
    {
        int status;

        waitpid(monitor_pid,
                &status,
                0);

        printf("IPC monitor process stopped.\n");

        monitor_pid = -1;
    }

    /*
     * Safety cleanup.
     */
    if (ipc_pipe[0] != -1)
    {
        close(ipc_pipe[0]);
        ipc_pipe[0] = -1;
    }
}
