#include "security.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

/*
 * Check whether the path contains a dangerous
 * parent-directory component.
 */
static int contains_parent_directory(const char *path)
{
    const char *current = path;

    while (*current != '\0')
    {
        /*
         * A path component exactly equal to ".."
         * is dangerous.
         */
        if (current[0] == '.' &&
            current[1] == '.' &&
            (current[2] == '/' ||
             current[2] == '\0'))
        {
            return 1;
        }

        /*
         * Move to the next path component.
         */
        current = strchr(current, '/');

        if (current == NULL)
        {
            break;
        }

        current++;
    }

    return 0;
}

/*
 * Reject control characters.
 *
 * Characters below ASCII 32 can cause unexpected
 * behaviour in HTTP or filesystem processing.
 */
static int contains_control_character(const char *path)
{
    for (const unsigned char *current =
             (const unsigned char *)path;
         *current != '\0';
         current++)
    {
        if (iscntrl(*current))
        {
            return 1;
        }
    }

    return 0;
}

int security_validate_path(const char *path)
{
    if (path == NULL)
    {
        return -1;
    }

    /*
     * An HTTP path should begin with '/'.
     */
    if (path[0] != '/')
    {
        return -1;
    }

    /*
     * Reject empty path components that attempt
     * to escape the web root using "..".
     */
    if (contains_parent_directory(path))
    {
        return -1;
    }

    /*
     * Reject control characters.
     */
    if (contains_control_character(path))
    {
        return -1;
    }

    /*
     * Reject paths containing a backslash.
     *
     * This keeps path handling predictable and
     * prevents Windows-style traversal attempts
     * from being accepted.
     */
    if (strchr(path, '\\') != NULL)
    {
        return -1;
    }

    return 0;
}
