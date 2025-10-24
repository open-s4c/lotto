/*
 */
/**
 * Definitions for implementing a trace. The implementation should provide
 * function pointers packed in struct trace.
 */
#ifndef LOTTO_TRACE_IMPL_H
#define LOTTO_TRACE_IMPL_H

#include <lotto/base/record.h>
#include <lotto/base/trace.h>
#include <lotto/sys/stream.h>

typedef int(trace_append_f)(trace_t *t, record_t *r);
typedef record_t *(trace_next_f)(trace_t *t, enum record filter);
typedef void(trace_advance_f)(trace_t *t);
typedef record_t *(trace_last_f)(trace_t *t);
typedef stream_t *(trace_stream_f)(trace_t *t);
typedef void(trace_forget_f)(trace_t *t);
typedef void(trace_clear_f)(trace_t *t);
typedef void(trace_load_f)(trace_t *t);
typedef void(trace_save_f)(const trace_t *t);
typedef void(trace_save_to_f)(const trace_t *t, stream_t *stream);

struct trace {
    trace_append_f *append;
    trace_append_f *append_safe;
    trace_next_f *next;
    trace_advance_f *advance;
    trace_last_f *last;
    trace_forget_f *forget;
    trace_clear_f *clear;
    trace_load_f *load;
    trace_save_f *save;
    trace_save_to_f *save_to;
    trace_stream_f *stream;
};

#endif
