#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

/*
 * Initialize MicroHTTP signal handling.
 *
 * Handles:
 * SIGINT  -> Ctrl+C
 * SIGTERM -> graceful termination request
 *
 * Returns:
 *  0  -> success
 * -1  -> failure
 */
int signal_handler_init(void);

/*
 * Check whether the server shutdown
 * has been requested.
 *
 * Returns:
 *  1 -> shutdown requested
 *  0 -> continue running
 */
int signal_handler_shutdown_requested(void);

/*
 * Reset the shutdown state.
 *
 * Mainly useful for controlled testing.
 */
void signal_handler_reset(void);

#endif
