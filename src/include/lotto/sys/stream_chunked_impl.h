/*
 */
/**
 * Definitions for implementing a multistream. The implementation should provide
 * function pointers packed in stream_chunked_t.
 */
#ifndef LOTTO_STREAM_MULTI_IMPL_H
#define LOTTO_STREAM_MULTI_IMPL_H

#include <stdint.h>

#include <lotto/sys/stream_chunked.h>
#include <lotto/sys/stream_impl.h>

typedef uint64_t(stream_chunked_nchunks_f)(stream_t *stream);
typedef bool(stream_chunked_set_chunk_f)(stream_t *stream, uint64_t chunk);
typedef bool(stream_chunked_next_chunk_f)(stream_t *stream);
typedef void(stream_chunked_copy_chunks_f)(const stream_t *source_stream,
                                           stream_t *sink_stream,
                                           uint64_t first_chunk,
                                           uint64_t last_chunk);
typedef void(stream_chunked_truncate_f)(const stream_t *stream,
                                        uint64_t chunk_limit);

typedef struct {
    stream_t s;
    stream_chunked_nchunks_f *nchunks;
    stream_chunked_set_chunk_f *set_chunk;
    stream_chunked_next_chunk_f *next_chunk;
    stream_chunked_copy_chunks_f *copy_chunks;
    stream_chunked_truncate_f *truncate;
} stream_chunked_t;

#endif
