#include "static_file.h"
#include "security.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *get_content_type(const char *path)
{
    const char *extension = strrchr(path, '.');

    if (extension == NULL)
    {
        return "application/octet-stream";
    }

    if (strcmp(extension, ".html") == 0 ||
        strcmp(extension, ".htm") == 0)
    {
        return "text/html";
    }

    if (strcmp(extension, ".css") == 0)
    {
        return "text/css";
    }

    if (strcmp(extension, ".js") == 0)
    {
        return "application/javascript";
    }

    if (strcmp(extension, ".txt") == 0)
    {
        return "text/plain";
    }

    if (strcmp(extension, ".json") == 0)
    {
        return "application/json";
    }

    if (strcmp(extension, ".png") == 0)
    {
        return "image/png";
    }

    if (strcmp(extension, ".jpg") == 0 ||
        strcmp(extension, ".jpeg") == 0)
    {
        return "image/jpeg";
    }

    if (strcmp(extension, ".gif") == 0)
    {
        return "image/gif";
    }

    return "application/octet-stream";
}

int static_file_read(const char *path,
                     char **file_data,
                     size_t *file_size,
                     const char **content_type)
{
    if (path == NULL ||
        file_data == NULL ||
        file_size == NULL ||
        content_type == NULL)
    {
        return -1;
    }

    /*
     * Security validation must happen before
     * constructing the filesystem path.
     */
    if (security_validate_path(path) != 0)
    {
        return -1;
    }

    /*
     * Defense-in-depth:
     * Reject parent-directory references even if
     * another validation layer changes later.
     */
    if (strstr(path, "..") != NULL)
    {
        return -1;
    }

    char file_path[4096];

    int path_length = snprintf(file_path,
                               sizeof(file_path),
                               "%s%s",
                               WEB_ROOT,
                               path);

    if (path_length < 0 ||
        (size_t)path_length >= sizeof(file_path))
    {
        return -1;
    }

    /*
     * The root URL "/" serves index.html.
     */
    if (strcmp(path, "/") == 0)
    {
        int root_path_length =
            snprintf(file_path,
                     sizeof(file_path),
                     "%s/index.html",
                     WEB_ROOT);

        if (root_path_length < 0 ||
            (size_t)root_path_length >= sizeof(file_path))
        {
            return -1;
        }
    }

    FILE *file = fopen(file_path, "rb");

    if (file == NULL)
    {
        return -1;
    }

    if (fseek(file, 0, SEEK_END) != 0)
    {
        fclose(file);
        return -1;
    }

    long size = ftell(file);

    if (size < 0)
    {
        fclose(file);
        return -1;
    }

    rewind(file);

    char *buffer = malloc((size_t)size + 1);

    if (buffer == NULL)
    {
        fclose(file);
        return -1;
    }

    size_t bytes_read =
        fread(buffer,
              1,
              (size_t)size,
              file);

    fclose(file);

    if (bytes_read != (size_t)size)
    {
        free(buffer);
        return -1;
    }

    buffer[bytes_read] = '\0';

    *file_data = buffer;
    *file_size = bytes_read;
    *content_type = get_content_type(file_path);

    return 0;
}
