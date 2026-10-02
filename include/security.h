#ifndef SECURITY_H
#define SECURITY_H

/*
 * Validate an HTTP request path before it is used
 * to access the filesystem.
 *
 * Returns:
 *  0  -> path is safe
 * -1  -> path is unsafe
 */
int security_validate_path(const char *path);

#endif
