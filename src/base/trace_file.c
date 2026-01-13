
/*
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/record.h>
#include <lotto/base/trace_file.h>
#include <lotto/base/trace_impl.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream.h>
#include <lotto/sys/string.h>

typedef struct {
    trace_t iface;
    record_t *cur;
    stream_t *stream;
} trace_file_t;

/**
 * @retval TRACE_ERROR some error while pushing
 */
static int
_append_safe(trace_t *t, record_t *record)
{
    if (t == NULL)
        return TRACE_ERROR;

    ASSERT(record != NULL);
    trace_file_t *tf = (trace_file_t *)t;

    // calculate total size of record
    size_t size = sizeof(record_t) + record->size;

    if (size != stream_write(tf->stream, (const char *)record, size))
        return TRACE_ERROR;

    return TRACE_OK;
}

/**
 * @retval TRACE_ERROR some error while pushing
 */
static int
_append(trace_t *t, record_t *record)
{
    int ret = _append_safe(t, record);
    if (ret == TRACE_OK) {
        sys_free(record);
    }
    return ret;
}

static void
_advance(trace_t *t)
{
    trace_file_t *tf = (trace_file_t *)t;
    if (tf->cur == NULL)
        return;

    sys_free(tf->cur);
    tf->cur = NULL;
}

static record_t *
_get_next(trace_file_t *tf, enum record filter)
{
    if (tf->cur && (tf->cur->kind & filter))
        return tf->cur;

    record_t header  = {0};
    const size_t hsz = sizeof(record_t);
    record_t *r      = NULL;
    while ((header.kind & filter) == 0) {
        if (r)
            sys_free(r);

        if (hsz != stream_read(tf->stream, (char *)&header, hsz))
            return NULL;

        r = record_alloc(header.size);
        sys_memcpy(r, &header, sizeof(record_t));
        size_t sz = stream_read(tf->stream, r->data, r->size);
        ASSERT(sz == r->size && "could not read payload");
    }
    return r;
}


static record_t *
_next(trace_t *t, enum record filter)
{
    if (t == NULL)
        return NULL;

    trace_file_t *tf = (trace_file_t *)t;

    tf->cur = _get_next(tf, filter);
    return tf->cur;
}


static stream_t *
_stream(trace_t *t)
{
    if (t == NULL)
        return NULL;
    trace_file_t *tf = (trace_file_t *)t;
    return tf->stream;
}

static void
_load(trace_t *t)
{
}

static record_t *
_last(trace_t *t)
{
    if (t == NULL)
        return NULL;

    trace_file_t *tf = (trace_file_t *)t;

    stream_reset(tf->stream);
    sys_free(tf->cur);
    tf->cur          = NULL;
    record_t *result = NULL;
    for (record_t *r = _get_next(tf, RECORD_ANY); r != NULL;
         r           = _get_next(tf, RECORD_ANY)) {
        sys_free(result);
        result = r;
    }
    tf->cur = result;
    return result;
}

static void
_clear(trace_t *t)
{
    if (t == NULL)
        return;

    trace_file_t *tf = (trace_file_t *)t;

    sys_free(tf->cur);
    tf->cur = NULL;
}

static void
_save_to(const trace_t *t, stream_t *stream)
{
    if (t == NULL)
        return;

    trace_file_t *tf = (trace_file_t *)t;
    sys_free(tf->cur);
    tf->cur = NULL;
    for (record_t *record = _next(&tf->iface, RECORD_ANY); record;
         _advance(&tf->iface), record = _next(&tf->iface, RECORD_ANY)) {
        size_t size = sizeof(record_t) + record->size;
        // dump record
        size_t r = stream_write(stream, (const char *)record, size);
        if (r == 0)
            logger_fatalf("failed to dump record");
    }
}

trace_t *
trace_file_create(stream_t *stream)
{
    trace_file_t *tf = (trace_file_t *)sys_malloc(sizeof(trace_file_t));
    ASSERT(tf && "failed to allocate recorder");
    tf->iface.advance     = _advance;
    tf->iface.append      = _append;
    tf->iface.append_safe = _append_safe;
    tf->iface.clear       = _clear;
    tf->iface.forget      = NULL;
    tf->iface.last        = _last;
    tf->iface.load        = _load;
    tf->iface.next        = _next;
    tf->iface.save        = NULL;
    tf->iface.save_to     = _save_to;
    tf->iface.stream      = _stream;
    tf->stream            = stream;
    tf->cur               = NULL;
    return &tf->iface;
}
