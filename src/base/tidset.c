/*
 */
#include <stdbool.h>
#include <stdlib.h> // qsort

#include <lotto/base/marshable.h>
#include <lotto/base/tidset.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

/*******************************************************************************
 * public interface
 ******************************************************************************/
void
tidset_init_cap(tidset_t *tset, size_t cap)
{
    ASSERT(tset);
    *tset = (tidset_t){
        .m        = MARSHABLE_TIDSET,
        .tasks    = (task_id *)sys_malloc(sizeof(task_id) * cap),
        .capacity = cap,
        .size     = 0,
    };
}

void
tidset_init(tidset_t *tset)
{
    tidset_init_cap(tset, TIDSET_INIT_SIZE);
}

void
tidset_fini(tidset_t *tset)
{
    ASSERT(tset);
    if (tset->tasks)
        sys_free(tset->tasks);
    sys_memset(tset, 0, sizeof(tidset_t));
}

size_t
tidset_size(const tidset_t *tset)
{
    ASSERT(tset);
    return tset->size;
}

void
tidset_clear(tidset_t *tset)
{
    ASSERT(tset);
    tset->size = 0;
}

task_id
tidset_get(const tidset_t *tset, size_t idx)
{
    ASSERT(tset);
    if (tset->size == 0)
        return NO_TASK;
    ASSERT(tset->tasks);

    if (idx >= tset->size)
        return NO_TASK;

    return tset->tasks[idx];
}

void
tidset_set(tidset_t *tset, size_t idx, task_id id)
{
    ASSERT(tset);
    if (tset->size == 0)
        return;

    ASSERT(tset->tasks);
    ASSERT(id != NO_TASK);
    if (idx >= tset->size)
        return;

    for (size_t i = 0; i < tset->size; i++) {
        if (tset->tasks[i] == id && i != idx) {
            tset->tasks[i] = tset->tasks[idx];
            break;
        }
    }

    tset->tasks[idx] = id;
}

bool
tidset_remove(tidset_t *tset, task_id id)
{
    ASSERT(tset);
    ASSERT(tset->size == 0 || tset->tasks);

    for (size_t i = 0; i < tset->size; i++) {
        if (tset->tasks[i] != id)
            continue;
        tset->tasks[i] = tset->tasks[--tset->size];
        return true;
    }
    return false;
}

bool
tidset_has(const tidset_t *tset, task_id id)
{
    ASSERT(tset);

    for (size_t i = 0; i < tset->size; i++)
        if (tset->tasks[i] == id)
            return true;
    return false;
}

void
tidset_expand(tidset_t *tset, size_t cap)
{
    ASSERT(tset);
    if (cap == 0)
        cap = TIDSET_INIT_SIZE;

    if (tset->tasks == NULL) {
        tidset_init_cap(tset, cap);
        return;
    }

    tidset_t tmp = {0};
    tidset_init_cap(&tmp, cap);
    sys_memcpy(tmp.tasks, tset->tasks, tset->size * sizeof(task_id));
    sys_free(tset->tasks);

    tset->capacity = tmp.capacity;
    tset->tasks    = tmp.tasks;
}

bool
tidset_insert(tidset_t *tset, task_id id)
{
    ASSERT(tset);
    ASSERT(tset->size == 0 || tset->tasks);
    if (tidset_has(tset, id)) {
        return false;
    }

    if (tset->size + 1 > tset->capacity) {
        tidset_expand(tset, tset->capacity * 2);
    }

    tset->tasks[tset->size++] = id;
    return true;
}

void
tidset_copy(tidset_t *dst, const tidset_t *src)
{
    ASSERT(dst);
    ASSERT(src);
    if (dst->capacity < src->capacity)
        tidset_expand(dst, src->capacity);
    dst->size = src->size;
    sys_memcpy(dst->tasks, src->tasks, sizeof(task_id) * src->size);
}

void
tidset_sort(tidset_t *tset, tidset_cmp_f cmp)
{
    qsort(tset->tasks, tset->size, sizeof(task_id), cmp);
}

void
tidset_intersect(tidset_t *tset1, const tidset_t *tset2)
{
    ASSERT(tset1);
    ASSERT(tset2);

    for (size_t i = 0; i < tset1->size;) {
        if (tidset_has(tset2, tset1->tasks[i])) {
            i++;
            continue;
        }
        tset1->tasks[i] = tset1->tasks[--tset1->size];
    }
}

void
tidset_subtract(tidset_t *tset1, const tidset_t *tset2)
{
    ASSERT(tset1);
    ASSERT(tset2);

    for (size_t i = 0; i < tset1->size;) {
        if (!tidset_has(tset2, tset1->tasks[i])) {
            i++;
            continue;
        }
        tset1->tasks[i] = tset1->tasks[--tset1->size];
    }
}

void
tidset_union(tidset_t *tset1, const tidset_t *tset2)
{
    ASSERT(tset1);
    ASSERT(tset2);

    for (size_t i = 0; i < tset2->size; i++) {
        tidset_insert(tset1, tidset_get(tset2, i));
    }
}

void
tidset_filter(tidset_t *tset, bool (*predicate)(task_id))
{
    ASSERT(tset);
    ASSERT(predicate);
    for (size_t i = 0; i < tset->size;) {
        if (predicate(tset->tasks[i])) {
            i++;
            continue;
        }
        tset->tasks[i] = tset->tasks[--tset->size];
    }
}

bool
tidset_contains_all(const tidset_t *tset1, const tidset_t *tset2)
{
    ASSERT(tset1);
    ASSERT(tset2);

    if (tset2->size == 0)
        return true;

    if (tset1->size == 0)
        return false;

    for (size_t i = 0; i < tset2->size; i++) {
        if (!tidset_has(tset1, tset2->tasks[i])) {
            return false;
        }
    }

    return true;
}

bool
tidset_equals(const tidset_t *tset1, const tidset_t *tset2)
{
    ASSERT(tset1);
    ASSERT(tset2);

    return tidset_contains_all(tset1, tset2) &&
           tidset_contains_all(tset2, tset1);
}

void
tidset_retain_all_keys(tidset_t *tset, const tidmap_t *tmap)
{
    ASSERT(tset);
    ASSERT(tmap);

    for (size_t i = 0; i < tset->size;) {
        if (tidmap_find(tmap, tset->tasks[i])) {
            i++;
            continue;
        }
        tset->tasks[i] = tset->tasks[--tset->size];
    }
}

void
tidset_remove_all_keys(tidset_t *tset, const tidmap_t *tmap)
{
    ASSERT(tset);
    ASSERT(tmap);

    if (tset->size == 0)
        return;

    for (size_t i = 0; i < tset->size;) {
        if (!tidmap_find(tmap, tset->tasks[i])) {
            i++;
            continue;
        }
        tset->tasks[i] = tset->tasks[--tset->size];
    }
}

void
tidset_make_first(tidset_t *tset, task_id id)
{
    ASSERT(tset);

    for (size_t i = 0; i < tidset_size(tset); i++) {
        if (tset->tasks[i] != id)
            continue;
        if (i == 0)
            return;
        tset->tasks[i] = tset->tasks[0];
        tset->tasks[0] = id;
        return;
    }
    ASSERT(0 && "the element does not exist");
}

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
size_t
tidset_msize(const marshable_t *m)
{
    ASSERT(m);
    tidset_t *tset = (tidset_t *)m;
    size_t off     = offsetof(marshable_t, payload);
    return sizeof(tidset_t) - off + sizeof(task_id) * tset->size;
}

void *
tidset_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b        = (char *)buf;
    tidset_t *tset = (tidset_t *)m;

    size_t off = offsetof(marshable_t, payload);
    size_t len = sizeof(tidset_t) - off;
    sys_memcpy(b, ((char *)tset->m.payload), len);
    b += len;

    len = sizeof(task_id) * tset->size;
    sys_memcpy(b, tset->tasks, len);

    return b + len;
}


const void *
tidset_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    tidset_t *tset = (tidset_t *)m;
    size_t cur_cap = tset->capacity;
    size_t off     = offsetof(marshable_t, payload);
    size_t len     = m->alloc_size - off;
    sys_memcpy(m->payload, buf, len - sizeof(task_id *));
    char *b = (char *)buf + len;


    ASSERT(tset->size <= tset->capacity);
    len = sizeof(task_id) * tset->capacity;
    if (cur_cap < tset->capacity) {
        sys_free(tset->tasks);
        tset->tasks = (task_id *)sys_malloc(len);
    } else {
        tset->capacity = cur_cap;
    }

    len = sizeof(task_id) * tset->size;
    sys_memcpy(tset->tasks, b, len);
    return b + len;
}

void
tidset_print(const marshable_t *m)
{
    ASSERT(m);
    const tidset_t *tset = (const tidset_t *)m;
    logger_printf("[");
    for (size_t i = 0; i < tset->size; i++) {
        if (i != 0)
            logger_printf(", ");
        logger_printf("%lu", tset->tasks[i]);
    }
    logger_printf("]\n");
}


int
tidset_cmp_lt(const void *a, const void *b)
{
    const task_id ta = *(const task_id *)a;
    const task_id tb = *(const task_id *)b;

    return ta < tb;
}

int
tidset_cmp_gt(const void *a, const void *b)
{
    const task_id ta = *(const task_id *)a;
    const task_id tb = *(const task_id *)b;

    return ta > tb;
}
