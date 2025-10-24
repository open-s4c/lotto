/*
 */
#ifndef LOTTO_STREAM_MULTI_FILE_H
#define LOTTO_STREAM_MULTI_FILE_H

#include <dirent.h>

#include <lotto/sys/stream_chunked.h>

/**
 * Allocates and initializes a multifile stream. Files are stored in dir_path
 * the same as the dp pointee. File names are indices starting from zero
 * appended with the suffix.
 *
 * @param dp directory pointer
 * @param dir_path directory path
 * @param dir_path directory path
 * @return pointer to a multifile stream
 */
stream_t *stream_chunked_file(DIR *dp, const char *dir_path,
                              const char *suffix);

#endif
