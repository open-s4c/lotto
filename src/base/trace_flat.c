/*
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/base/record.h>
#include <lotto/base/trace_flat.h>
#include <lotto/base/trace_impl.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/stream.h>
#include <lotto/sys/string.h>

typedef struct trace_flat_node_s {
    struct trace_flat_node_s *prev;
    struct trace_flat_node_s *next;
    record_t *record;
} trace_flat_node_t;

typedef struct {
    trace_t iface;
    trace_flat_node_t *head;
    trace_flat_node_t *tail;
    stream_t *stream;
} trace_flat_t;

static trace_flat_node_t *
_make_node(record_t *record)
{
    trace_flat_node_t *result = sys_malloc(sizeof(trace_flat_node_t));
    *result                   = (trace_flat_node_t){.record = record};
    return result;
}

/**
 * @retval TRACE_ERROR some error while pushing
 */
static int
_append(trace_t *t, record_t *record)
{
    if (t == NULL)
        return TRACE_ERROR;

    trace_flat_t *tf        = (trace_flat_t *)t;
    trace_flat_node_t *node = _make_node(record);
    if (tf->head == NULL)
        tf->head = node;
    else
        tf->tail->next = node;
    node->prev = tf->tail;
    tf->tail   = node;

    return TRACE_OK;
}

static record_t *
_pop(trace_t *t)
{
    if (t == NULL)
        return NULL;
    trace_flat_t *tf = (trace_flat_t *)t;

    trace_flat_node_t *head = tf->head;
    if (head == NULL)
        return NULL;
    tf->head = head->next;
    if (tf->head) {
        tf->head->prev = NULL;
    }
    if (tf->tail == head)
        tf->tail = tf->head;
    record_t *result = head->record;
    sys_free(head);
    return result;
}

static void
_advance(trace_t *t)
{
    record_t *record = _pop(t);
    if (record) {
        sys_free(record);
    }
}

static void
_clear(trace_t *t)
{
    if (t == NULL)
        return;

    trace_flat_t *tf = (trace_flat_t *)t;
    if (tf->head != NULL)
        while (trace_next(t, RECORD_ANY))
            trace_advance(t);
    ASSERT(tf->head == NULL);
    ASSERT(tf->tail == NULL);
}

static record_t *
_next(trace_t *t, enum record filter)
{
    if (t == NULL)
        return NULL;

    trace_flat_t *tf        = (trace_flat_t *)t;
    trace_flat_node_t *node = tf->head;
    while (node && (node->record->kind & filter) == 0) {
        trace_advance(t);
        node = tf->head;
    }
    return node ? node->record : NULL;
}

static void
_load(trace_t *t)
{
    if (t == NULL)
        return;

    trace_flat_t *tf = (trace_flat_t *)t;
    // read record header from stream
    record_t header;
    const size_t header_size = sizeof(record_t);
    size_t r;
    while ((r = stream_read(tf->stream, (char *)&header, header_size)) ==
           header_size) {
        // create record with enough space
        record_t *record = record_alloc(header.size);
        // copy header
        sys_memcpy(record, &header, sizeof(record_t));
        // read data from stream
        r = stream_read(tf->stream, record->data, record->size);
        ASSERT(r == record->size && "could not read payload");
        if (TRACE_OK != trace_append(t, record))
            ASSERT(0 && "trace append failed");
    }
    // the last read should have left no pending data
    ASSERT(r == 0);
}

static void
_save(const trace_t *t)
{
    trace_flat_t *tf = (trace_flat_t *)t;
    trace_save_to(t, tf->stream);
}

static void
_save_to(const trace_t *t, stream_t *stream)
{
    if (t == NULL)
        return;

    trace_flat_t *tf = (trace_flat_t *)t;
    for (trace_flat_node_t *node = tf->head; node; node = node->next) {
        record_t *record = node->record;
        // calculate total size of record
        size_t size = sizeof(record_t) + record->size;
        // dump record
        size_t r = stream_write(stream, (const char *)record, size);
        if (r == 0)
            log_fatalf("failed to dump record");
    }
}

static record_t *
_last(trace_t *t)
{
    if (t == NULL)
        return NULL;

    trace_flat_t *tf = (trace_flat_t *)t;
    return tf->tail ? tf->tail->record : NULL;
}

static void
_forget(trace_t *t)
{
    if (t == NULL) {
        return;
    }
    trace_flat_t *tf        = (trace_flat_t *)t;
    trace_flat_node_t *tail = tf->tail;
    if (tail == NULL) {
        return;
    }
    if (tail == tf->head) {
        tf->tail = tf->head = NULL;
    } else {
        tf->tail       = tail->prev;
        tf->tail->next = NULL;
    }
    sys_free(tail->record);
    sys_free(tail);
}

static stream_t *
_stream(trace_t *t)
{
    if (t == NULL) {
        return NULL;
    }
    trace_flat_t *tf = (trace_flat_t *)t;
    return tf->stream;
}

trace_t *
trace_flat_create(stream_t *stream)
{
    trace_flat_t *tf = (trace_flat_t *)sys_malloc(sizeof(trace_flat_t));
    ASSERT(tf && "failed to allocate recorder");
    tf->iface.advance     = _advance;
    tf->iface.append      = _append;
    tf->iface.append_safe = _append;
    tf->iface.clear       = _clear;
    tf->iface.forget      = _forget;
    tf->iface.last        = _last;
    tf->iface.load        = _load;
    tf->iface.next        = _next;
    tf->iface.save        = _save;
    tf->iface.save_to     = _save_to;
    tf->iface.stream      = _stream;
    tf->head              = NULL;
    tf->tail              = NULL;
    tf->stream            = stream;
    return &tf->iface;
}
