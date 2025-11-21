/*
 */
#ifndef LOTTO_TRACE_FLAT_H
#define LOTTO_TRACE_FLAT_H

#include <lotto/base/trace.h>
#include <lotto/sys/stream.h>

/**
 * Allocates and initialize a new trace.
 *
 * @param stream stream pointer
 * @return A new and initialized trace object.
 */
trace_t *trace_flat_create(stream_t *stream);

#endif
