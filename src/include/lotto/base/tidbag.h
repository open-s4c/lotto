/*
 */
#ifndef LOTTO_TIDBAG_H
#define LOTTO_TIDBAG_H

#include <stdbool.h>

#include <lotto/base/marshable.h>
#include <lotto/base/task_id.h>

typedef struct tidbag {
    marshable_t m;
    size_t capacity;
    size_t size;
    task_id *tasks;
} tidbag_t;

#define TIDBAG_INIT_SIZE 4

/*******************************************************************************
 * marshaling interface
 ******************************************************************************/
void tidbag_print(const marshable_t *m);
size_t tidbag_msize(const marshable_t *m);
void *tidbag_marshal(const marshable_t *m, void *buf);
const void *tidbag_unmarshal(marshable_t *m, const void *buf);

#define MARSHABLE_TIDBAG                                                       \
    (marshable_t)                                                              \
    {                                                                          \
        .alloc_size = sizeof(tidbag_t), .unmarshal = tidbag_unmarshal,         \
        .marshal = tidbag_marshal, .size = tidbag_msize, .print = tidbag_print \
    }

/*******************************************************************************
 * public interface
 ******************************************************************************/
void tidbag_init_cap(tidbag_t *tbag, size_t cap);
void tidbag_init(tidbag_t *tbag);
void tidbag_fini(tidbag_t *tbag);

size_t tidbag_size(const tidbag_t *tbag);
void tidbag_clear(tidbag_t *tbag);
task_id tidbag_get(const tidbag_t *tbag, size_t idx);
void tidbag_set(tidbag_t *tbag, size_t idx, task_id id);
void tidbag_remove(tidbag_t *tbag, task_id id);
bool tidbag_has(const tidbag_t *tbag, task_id id);
void tidbag_expand(tidbag_t *tbag, size_t cap);
void tidbag_insert(tidbag_t *tbag, task_id id);
void tidbag_copy(tidbag_t *dst, const tidbag_t *src);
bool tidbag_equals(const tidbag_t *tbag1, const tidbag_t *tbag2);

void tidbag_filter(tidbag_t *tbag, bool (*predicate)(task_id));

#endif
