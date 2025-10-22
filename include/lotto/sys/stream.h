/**
 * A generic interface for i/o streams.
 */
#ifndef LOTTO_STREAM_H
#define LOTTO_STREAM_H

#include <stddef.h>

typedef struct stream_s stream_t;

/**
 * Writes buf to the stream.
 *
 * @param stream stream pointer
 * @param buf buffer to be written
 * @param size buffer size
 * @return size of written data
 */
size_t stream_write(stream_t *stream, const char *buf, size_t size);
/**
 * Reads buf from the stream.
 *
 * @param stream stream pointer
 * @param buf buffer to be read
 * @param size buffer size
 * @return size of read data
 */
size_t stream_read(stream_t *stream, char *buf, size_t size);
/**
 * Closes the stream.
 *
 * @param stream stream pointer
 */
void stream_close(stream_t *stream);
/**
 * Resets the stream.
 *
 * @param stream stream pointer
 */
void stream_reset(stream_t *stream);

#endif
