/*
 */
#ifndef LOTTO_STREAM_FILE_H
#define LOTTO_STREAM_FILE_H

#include <stdio.h>

#include <lotto/sys/stream.h>

/**
 * Allocates a file stream.
 *
 * @return pointer to a file stream
 */
stream_t *stream_file_alloc(void);

/**
 * Initializes the file stream.
 *
 * @param s file stream pointer
 * @param fd file descriptor
 */
void stream_file_init(stream_t *s, int fd);

/**
 * Opens file reading and initializes the file stream.
 *
 * @param s pointer to a file stream
 * @param fname filename
 */
void stream_file_in(stream_t *s, const char *fname);

/**
 * Opens file for writing (truncate) and initializes the file stream.
 *
 * @param s pointer to a file stream
 * @param fname filename
 */
void stream_file_out(stream_t *s, const char *fname);

#endif
