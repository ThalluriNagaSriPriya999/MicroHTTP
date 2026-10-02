#ifndef IPC_H
#define IPC_H

#include <stddef.h>

/*
 * IPC message types exchanged between
 * the MicroHTTP server and monitor process.
 */
#define IPC_EVENT_CONNECTION 1
#define IPC_EVENT_REQUEST 2
#define IPC_EVENT_SHUTDOWN 3

/*
 * Maximum size of an IPC message.
 */
#define IPC_MESSAGE_MAX 256

/*
 * IPC message structure.
 */
typedef struct
{
    int event_type;
    int status_code;
    unsigned long worker_id;
    char client_ip[64];
    char path[128];

} ipc_message_t;

/*
 * Create the IPC pipe.
 *
 * Returns:
 *   0  -> success
 *  -1  -> failure
 */
int ipc_create(void);

/*
 * Get the write end of the IPC pipe.
 */
int ipc_get_write_fd(void);

/*
 * Get the read end of the IPC pipe.
 */
int ipc_get_read_fd(void);

/*
 * Send an IPC message through the pipe.
 */
int ipc_send_message(const ipc_message_t *message);

/*
 * Start the monitor child process.
 */
int ipc_start_monitor(void);

/*
 * Close IPC resources.
 */
void ipc_shutdown(void);

#endif
