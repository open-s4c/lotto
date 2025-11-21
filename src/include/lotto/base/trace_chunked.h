/*
 */
#ifndef LOTTO_TRACE_CHUNKED_H
#define LOTTO_TRACE_CHUNKED_H

/*******************************************************************************
 * @file trace_chunked.h
 * @brief a chunked trace implementation
 *
 * A multirecorder uses a multistream to split records accross stream chunks.
 * Each chunk will have chunk_size records. The last chunk may have fewer
 * records. If the recorder needs to be read, it should be initialized with the
 * same chunk size as the input stream.
 *
 ******************************************************************************/

#include <lotto/base/trace.h>
#include <lotto/sys/stream.h>

/**
 * Allocates and initialize a new chunked trace.
 *
 * @return A new and initialized trace object.
 */
trace_t *trace_chunked_create(stream_t *stream_chunked, uint64_t chunk_size);

#endif
