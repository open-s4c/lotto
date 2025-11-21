/*
 */
/**
 * Definitions for implementing a stream. The implementation should provide
 * function pointers packed in stream_t.
 */
#ifndef LOTTO_STREAM_IMPL_H
#define LOTTO_STREAM_IMPL_H

#include <lotto/sys/stream.h>

typedef size_t(stream_write_f)(stream_t *stream, const char *buf, size_t size);
typedef size_t(stream_read_f)(stream_t *stream, char *buf, size_t size);
typedef void(stream_close_f)(stream_t *stream);
typedef void(stream_reset_f)(stream_t *stream);

struct stream_s {
    stream_write_f *write;
    stream_read_f *read;
    stream_close_f *close;
    stream_reset_f *reset;
};

#endif
