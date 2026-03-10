#ifndef LOTTO_DRIVER_FILES_H
#define LOTTO_DRIVER_FILES_H

#include <stdbool.h>

typedef struct driver_file_s {
    const char *path;
    const unsigned char *content;
    unsigned int len;
} driver_file_t;

bool driver_try_dump_files(const char *dir, const driver_file_t files[]);
void driver_dump_files(const char *dir, const driver_file_t files[]);

#endif
