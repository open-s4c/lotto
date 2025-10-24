/*
 */
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/trace_chunked.h>
#include <lotto/base/trace_impl.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream_chunked.h>
#include <lotto/sys/string.h>
#include <sys/stat.h>

typedef struct {
    trace_t r;
    record_t *head;
    record_t *tail;
    stream_t *stream;
    uint64_t chunk_size;
    uint64_t current_chunk;
    uint64_t start_chunk;
    uint64_t nchunks;
    uint64_t current_size;
    uint64_t last_record_index;
    bool backed;
} trace_chunked_t;

static void _flush_chunk(trace_chunked_t *trace, stream_t *stream);
static void _set_chunk(trace_chunked_t *trace, uint64_t chunk);

static void
_load_chunk(trace_chunked_t *trace)
{
    if (!trace->backed) {
        _flush_chunk(trace, trace->stream);
    }
    trace_clear(&trace->r);
    // read record header from stream
    record_t header;
    const size_t header_size = sizeof(record_t);
    size_t r;
    while ((r = stream_read(trace->stream, (char *)&header, header_size)) ==
           header_size) {
        // create record with enough space
        record_t *record = record_alloc(header.size);
        // copy header
        sys_memcpy(record, &header, sizeof(record_t));
        // read data from stream
        r = stream_read(trace->stream, record->data, record->size);
        ASSERT(r == record->size && "could not read payload");
        int v = trace_append(&trace->r, record);
        ASSERT(v == TRACE_OK);
    }
    trace->backed = true;
    // the last read should have left no pending data
    ASSERT(r == 0);
    ASSERT(trace->current_size == trace->chunk_size || trace->nchunks <= 1 ||
           trace->current_chunk + 1 == trace->nchunks);
}

static void
_load_next_chunk(trace_chunked_t *trace)
{
    if (trace->nchunks == 0) {
        _load_chunk(trace);
        return;
    }
    if (trace->current_chunk + 1 < trace->nchunks) {
        _set_chunk(trace, trace->current_chunk + 1);
    }
}

static void
_next_chunk(trace_chunked_t *trace)
{
    if (!stream_chunked_next_chunk(trace->stream))
        ASSERT(0 && "next_chunk failed");
    trace->current_chunk++;
}

static void
_flush_chunk(trace_chunked_t *trace, stream_t *stream)
{
    trace->backed = true;
    if (trace->head == NULL) {
        return;
    }
    for (record_t *record = trace->head; record; trace->current_size--) {
        // calculate total size of record
        size_t size = sizeof(record_t) + record->size;

        // remove non-deterministic pointer value
        record_t *next = record->next;
        record->next   = NULL;

        // dump record
        size_t r = stream_write(stream, (const char *)record, size);
        if (r == 0)
            log_fatalf("failed to dump record");

        // restore
        record->next = next;

        record_t *prev = record;
        record         = record->next;
        sys_free(prev);
    }
    ASSERT(trace->current_size == 0);
    trace->head = NULL;
    trace->tail = NULL;
    if (trace->current_chunk == trace->nchunks) {
        trace->nchunks++;
    }
}

/**
 * @retval TRACE_ERROR some error while pushing
 */
static int
_append(trace_t *trace, record_t *record)
{
    if (trace == NULL)
        return TRACE_ERROR;

    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    if (trace_chunked->current_size == trace_chunked->chunk_size) {
        _flush_chunk(trace_chunked, trace_chunked->stream);
        _next_chunk(trace_chunked);
    }
    if (trace_chunked->head == NULL)
        trace_chunked->head = record;
    else
        trace_chunked->tail->next = record;
    trace_chunked->tail = record;
    trace_chunked->current_size++;
    record->next          = NULL; // TODO: use canary here
    trace_chunked->backed = false;

    return TRACE_OK;
}

static record_t *
_peek(trace_chunked_t *trace)
{
    if (trace->current_chunk > trace->start_chunk) {
        _set_chunk(trace, trace->start_chunk);
    }
    record_t *record = trace->head;
    if (record == NULL) {
        _load_next_chunk(trace);
        trace->start_chunk = trace->current_chunk;
    }
    return trace->head;
}

static void
_set_chunk(trace_chunked_t *trace, uint64_t chunk)
{
    ASSERT(trace->current_chunk != chunk);
    if (!trace->backed) {
        _flush_chunk(trace, trace->stream);
    }
    if (!stream_chunked_set_chunk(trace->stream, chunk))
        ASSERT(0 && "set_chunk failed");
    trace->current_chunk = chunk;
    _load_chunk(trace);
}

static record_t *
_pop(trace_chunked_t *trace)
{
    record_t *record = _peek(trace);
    trace->head      = record->next;
    if (trace->tail == record)
        trace->tail = trace->head;
    trace->current_size--;
    trace->backed = false;
    return record;
}

static void
_advance(trace_t *trace)
{
    if (trace == NULL)
        return;

    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    record_t *record               = _pop(trace_chunked);
    if (record)
        sys_free(record);
}

static void
_clear(trace_t *trace)
{
    if (trace == NULL)
        return;

    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    record_t *record               = trace_chunked->head;
    while (record) {
        record_t *record_next = record->next;
        sys_free(record);
        record = record_next;
        trace_chunked->current_size--;
    }
    trace_chunked->head = NULL;
    trace_chunked->tail = NULL;
    ASSERT(trace_chunked->head == NULL);
    ASSERT(trace_chunked->tail == NULL);
    ASSERT(trace_chunked->current_size == 0);
}

static record_t *
_next(trace_t *trace, enum record filter)
{
    if (trace == NULL)
        return NULL;

    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;

    record_t *r = _peek(trace_chunked);
    while (r && (r->kind & filter) == 0) {
        trace_advance(trace);
        r = _peek(trace_chunked);
    }
    return r;
}

static void
_load(trace_t *trace)
{
    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    _load_chunk(trace_chunked);
    trace_chunked->nchunks = stream_chunked_nchunks(trace_chunked->stream);
    ASSERT(trace_chunked->chunk_size == trace_chunked->current_size ||
           trace_chunked->nchunks == 0 ||
           (trace_chunked->nchunks == 1 &&
            trace_chunked->chunk_size > trace_chunked->current_size));
}

static void
_save(const trace_t *trace)
{
    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    _flush_chunk(trace_chunked, trace_chunked->stream);
    stream_chunked_truncate(trace_chunked->stream, trace_chunked->nchunks);
}

static void
_save_to(const trace_t *trace, stream_t *stream)
{
    _save(trace);
    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    if (trace_chunked->nchunks > 0) {
        stream_chunked_copy_chunks(trace_chunked->stream, stream, 0,
                                   trace_chunked->nchunks - 1);
    }
    stream_chunked_truncate(stream, trace_chunked->nchunks);
}

static record_t *
_last(trace_t *trace)
{
    if (trace == NULL)
        return NULL;
    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    if (trace_chunked->current_chunk + 1 >= trace_chunked->nchunks) {
        if (trace_chunked->tail == NULL && trace_chunked->current_chunk > 0) {
            _set_chunk(trace_chunked, trace_chunked->current_chunk - 1);
        }
        return trace_chunked->tail;
    }
    _set_chunk(trace_chunked, trace_chunked->nchunks - 1);
    return trace_chunked->tail;
}

static void
_forget(trace_t *trace)
{
    if (trace == NULL)
        return;
    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;

    if (trace_chunked->current_chunk + 1 < trace_chunked->nchunks) {
        _set_chunk(trace_chunked, trace_chunked->nchunks - 1);
    }
    if (trace_chunked->head == NULL) {
        if (trace_chunked->current_chunk == 0) {
            ASSERT(trace_chunked->tail == NULL);
            return;
        }
        _set_chunk(trace_chunked, trace_chunked->current_chunk - 1);
        trace_chunked->nchunks--;
    }

    record_t *cur  = trace_chunked->head;
    record_t *prev = cur;

    for (; cur && cur->next != NULL; prev = cur, cur = cur->next)
        ;
    ASSERT(cur);
    ASSERT(cur == trace_chunked->tail);
    sys_free(cur);
    trace_chunked->current_size--;

    if (cur != prev) {
        trace_chunked->tail = prev;
        prev->next          = NULL;
    } else {
        ASSERT(cur == trace_chunked->head);
        trace_chunked->head = NULL;
        trace_chunked->tail = NULL;
    }
    if (trace_chunked->current_size == 0 && trace_chunked->current_chunk) {
        _set_chunk(trace_chunked, trace_chunked->current_chunk - 1);
        trace_chunked->nchunks--;
    }
}

static stream_t *
_stream(trace_t *t)
{
    if (t == NULL)
        return NULL;
    trace_chunked_t *tc = (trace_chunked_t *)t;
    return tc->stream;
}

trace_t *
trace_chunked_create(stream_t *stream_chunked, uint64_t chunk_size)
{
    trace_t *trace = sys_malloc(sizeof(trace_t) + sizeof(trace_chunked_t));
    ASSERT(trace && "failed to allocate trace");
    trace->advance                 = _advance;
    trace->append                  = _append;
    trace->append_safe             = _append;
    trace->clear                   = _clear;
    trace->forget                  = _forget;
    trace->last                    = _last;
    trace->load                    = _load;
    trace->next                    = _next;
    trace->save                    = _save;
    trace->save_to                 = _save_to;
    trace->stream                  = _stream;
    trace_chunked_t *trace_chunked = (trace_chunked_t *)trace;
    trace_chunked->head            = NULL;
    trace_chunked->tail            = NULL;
    trace_chunked->stream          = stream_chunked;
    ASSERT(chunk_size > 0);
    trace_chunked->chunk_size    = chunk_size;
    trace_chunked->current_chunk = 0;
    trace_chunked->start_chunk   = 0;
    trace_chunked->nchunks       = 0;
    trace_chunked->current_size  = 0;
    trace_chunked->backed        = true;
    return trace;
}
