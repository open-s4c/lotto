/*
 */
#include <lotto/sys/stream_chunked_impl.h>
#include <lotto/util/redirect.h>

REDIRECT_CAST_RETURN(stream_chunked, uint64_t, nchunks, (stream_t * stream),
                     stream_chunked_t, stream, stream)
REDIRECT_CAST_RETURN(stream_chunked, bool, set_chunk,
                     (stream_t * stream, uint64_t chunk), stream_chunked_t,
                     stream, stream, chunk)
REDIRECT_CAST_RETURN(stream_chunked, bool, next_chunk, (stream_t * stream),
                     stream_chunked_t, stream, stream)
REDIRECT_CAST_VOID(stream_chunked, copy_chunks,
                   (const stream_t *source_stream, stream_t *sink_stream,
                    uint64_t first_chunk, uint64_t last_chunk),
                   stream_chunked_t, source_stream, source_stream, sink_stream,
                   first_chunk, last_chunk)
REDIRECT_CAST_VOID(stream_chunked, truncate,
                   (const stream_t *stream, uint64_t chunk_limit),
                   stream_chunked_t, stream, stream, chunk_limit)
