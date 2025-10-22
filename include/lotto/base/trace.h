#ifndef LOTTO_TRACE_H
#define LOTTO_TRACE_H

#include <lotto/base/record.h>
#include <lotto/sys/stream.h>

typedef struct trace trace_t;

#define TRACE_ERROR 0
#define TRACE_OK    1

/**
 * Appends a new record.
 *
 * The new record is added to the end of the trace. The trace object
 * owns the record pointer, the user cannot use or free that pointer after
 * passing it to the trace.
 *
 * @param t trace object
 * @param r record object
 *
 * @returns TRACE_ERROR or TRACE_OK
 */
int trace_append(trace_t *t, record_t *r);

/**
 * Appends a new record in a signal safe manner.
 *
 * The new record is added to the end of the trace. The trace object
 * owns the record pointer, the user cannot use or free that pointer after
 * passing it to the trace.
 *
 * @param t trace object
 * @param r record object
 *
 * @returns TRACE_ERROR or TRACE_OK
 */
int trace_append_safe(trace_t *t, record_t *r);

/**
 * Returns the next record.
 *
 * The returned record is still owned by the trace, the user cannot modify or
 * free the pointed memory location. If the user passes a filter, the trace
 * advances until a record matching the filter is found and returns it.
 *
 * @param t trace object
 * @param filter a filter for the record type.
 * @returns The next record matching the `filter` or NULL if no matching record
 * was found.
 */
record_t *trace_next(trace_t *t, enum record filter);

/**
 * Advances the trace.
 *
 * While the trace is not advanced by the user, `trace_next()` returns
 * always the same element (if the `filter` value is kept constant).
 *
 * @param t trace object
 */
void trace_advance(trace_t *t);

/**
 * Returns the last record.
 *
 * The returned record is still owned by the trace, the user cannot modify or
 * free the pointed memory location.
 *
 * @param t trace object
 * @returns The last record of the trace.
 */
record_t *trace_last(trace_t *t);

/**
 * Removes the last record.
 *
 * If the trace is non-empty, the last record is removed from the trace
 * and freed.
 *
 * @param t trace object
 */
void trace_forget(trace_t *t);

/**
 * Removes all records.
 *
 * All records in the trace are removed and freed, the trace object,
 * however, is not freed. This function can be used to reuse a trace instead
 * of creating a new one.
 *
 * @param t trace object
 */
void trace_clear(trace_t *t);

/**
 * Clears and frees a trace.
 *
 * @param t trace object
 */
void trace_destroy(trace_t *t);

/**
 * Loads a trace.
 * @param t trace object
 */
void trace_load(trace_t *t);

/**
 * Returns the stream object used to load or store the trace.
 * @param t trace object
 * @return stream object (might be NULL)
 */
stream_t *trace_stream(trace_t *t);

/**
 * Saves a trace.
 *
 * @param t trace object
 */
void trace_save(const trace_t *t);

/**
 * Saves a trace to the provided stream.
 *
 * @param rec    trace pointer
 * @param stream stream pointer
 */
void trace_save_to(const trace_t *t, stream_t *stream);

#endif
