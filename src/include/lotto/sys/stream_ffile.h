/*
 */
#ifndef LOTTO_STREAM_FFILE_H
#define LOTTO_STREAM_FFILE_H

#include <stdio.h>

#include <lotto/sys/stream.h>

/**
 * Allocates a ffile stream.
 *
 * @return pointer to a ffile stream
 */
stream_t *stream_ffile_alloc(void);

/**
 * Initializes the ffile stream.
 *
 * @param s ffile stream pointer
 * @param fp file pointer
 * @return pointer to a file stream
 */
void stream_ffile_init(stream_t *s, FILE *fp);

/**
 * Opens file reading and initializes the ffile stream.
 *
 * @param s pointer to a ffile stream
 * @param fname filename
 */
void stream_ffile_in(stream_t *s, const char *fname);

/**
 * Opens file for writing (truncate) and initializes the ffile stream.
 *
 * @param s pointer to a ffile stream
 * @param fname filename
 */
void stream_ffile_out(stream_t *s, const char *fname);

#endif
