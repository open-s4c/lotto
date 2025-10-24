/*
 */
/**
 * A multistream is a generic extension of streams allowing the data to be split
 * accross multiple sequential chunks.
 */
#ifndef LOTTO_STREAM_MULTI_H
#define LOTTO_STREAM_MULTI_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/sys/stream.h>

#define DEFAULT_TRACE_EXT ".trace"

/**
 * Returns the current number of chunks.
 *
 * @param stream multistream pointer
 * @return current number of chunks
 */
uint64_t stream_chunked_nchunks(stream_t *stream);
/**
 * Selects an existing chunk.
 *
 * @param stream multistream pointer
 * @param chunk chunk index
 * @return true if the chunk exists, false otherwise
 */
bool stream_chunked_set_chunk(stream_t *stream, uint64_t chunk);
/**
 * Selects the next existing chunk. If the chunk does not exist, creates a new
 * one for writing.
 *
 * @param stream multistream pointer
 * @return true if successful, false otherwise
 */
bool stream_chunked_next_chunk(stream_t *stream);
/**
 * Transfers chunk from the source stream to the sink stream and updates the
 * sink stream metadata. Chunks must exist in the source stream.
 *
 * @param source_stream source stream pointer
 * @param sink_stream sink stream pointer
 * @param first_chunk the first chunk to be copied
 * @param first_chunk the last chunk to be copied, must not be smaller than
 * first_chunk
 */
void stream_chunked_copy_chunks(const stream_t *source_stream,
                                stream_t *sink_stream, uint64_t first_chunk,
                                uint64_t last_chunk);
/**
 * Truncates the multistream by deleting all chunks with indices smaller than
 * chunk_limit. The current chunk is set to the last undeleted one.
 *
 * @param stream stream pointer
 * @param chunk_limit the first chunk to be removed
 */
void stream_chunked_truncate(const stream_t *stream, uint64_t chunk_limit);

#endif
