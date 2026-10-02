#ifndef STATIC_FILE_H
#define STATIC_FILE_H

#include <stddef.h>

#define WEB_ROOT "www"

int static_file_read(const char *path,
                     char **file_data,
                     size_t *file_size,
                     const char **content_type);

#endif
