/*
 */
#ifndef LOTTO_TIDSET_H
#define LOTTO_TIDSET_H

#include <stdbool.h>

#include <lotto/base/marshable.h>
#include <lotto/base/task_id.h>
#include <lotto/base/tidmap.h>

typedef struct tidset {
    marshable_t m;
    size_t capacity;
    size_t size;
    task_id *tasks;
} tidset_t;

#define TIDSET_INIT_SIZE 4

/*******************************************************************************
 * marshaling interface
 ******************************************************************************/
void tidset_print(const marshable_t *m);
size_t tidset_msize(const marshable_t *m);
void *tidset_marshal(const marshable_t *m, void *buf);
const void *tidset_unmarshal(marshable_t *m, const void *buf);

#define MARSHABLE_TIDSET                                                       \
    (marshable_t)                                                              \
    {                                                                          \
        .alloc_size = sizeof(tidset_t), .unmarshal = tidset_unmarshal,         \
        .marshal = tidset_marshal, .size = tidset_msize, .print = tidset_print \
    }

/*******************************************************************************
 * public interface
 ******************************************************************************/
void tidset_init_cap(tidset_t *tset, size_t cap);
void tidset_init(tidset_t *tset);
void tidset_fini(tidset_t *tset);

size_t tidset_size(const tidset_t *tset);
void tidset_clear(tidset_t *tset);
task_id tidset_get(const tidset_t *tset, size_t idx);
void tidset_set(tidset_t *tset, size_t idx, task_id id);
bool tidset_remove(tidset_t *tset, task_id id);
bool tidset_has(const tidset_t *tset, task_id id);
void tidset_expand(tidset_t *tset, size_t cap);
bool tidset_insert(tidset_t *tset, task_id id);
void tidset_copy(tidset_t *dst, const tidset_t *src);
bool tidset_equals(const tidset_t *tset1, const tidset_t *tset2);

typedef int (*tidset_cmp_f)(const void *a, const void *b);
int tidset_cmp_lt(const void *a, const void *b);
int tidset_cmp_gt(const void *a, const void *b);
void tidset_sort(tidset_t *tset, tidset_cmp_f cmp);

void tidset_union(tidset_t *tset1, const tidset_t *tset2);
void tidset_intersect(tidset_t *tset1, const tidset_t *tset2);
void tidset_subtract(tidset_t *tset1, const tidset_t *tset2);
void tidset_filter(tidset_t *tset, bool (*predicate)(task_id));
bool tidset_contains_all(const tidset_t *tset1, const tidset_t *tset2);

void tidset_retain_all_keys(tidset_t *tset, const tidmap_t *tmap);
void tidset_remove_all_keys(tidset_t *tset, const tidmap_t *tmap);

void tidset_make_first(tidset_t *tset, task_id id);

#endif
