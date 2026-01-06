/*
 */
#include <stdbool.h>

#include <lotto/base/marshable.h>
#include <lotto/base/tidbag.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

/*******************************************************************************
 * public interface
 ******************************************************************************/
void
tidbag_init_cap(tidbag_t *tbag, size_t cap)
{
    ASSERT(tbag);
    *tbag = (tidbag_t){
        .m        = MARSHABLE_TIDBAG,
        .tasks    = (task_id *)sys_malloc(sizeof(task_id) * cap),
        .capacity = cap,
        .size     = 0,
    };
}

void
tidbag_init(tidbag_t *tbag)
{
    tidbag_init_cap(tbag, TIDBAG_INIT_SIZE);
}

void
tidbag_fini(tidbag_t *tbag)
{
    ASSERT(tbag);
    if (tbag->tasks)
        sys_free(tbag->tasks);
    sys_memset(tbag, 0, sizeof(tidbag_t));
}

size_t
tidbag_size(const tidbag_t *tbag)
{
    ASSERT(tbag);
    return tbag->size;
}

void
tidbag_clear(tidbag_t *tbag)
{
    ASSERT(tbag);
    tbag->size = 0;
}

task_id
tidbag_get(const tidbag_t *tbag, size_t idx)
{
    ASSERT(tbag);
    if (tbag->size == 0)
        return NO_TASK;
    ASSERT(tbag->tasks);

    if (idx >= tbag->size)
        return NO_TASK;

    return tbag->tasks[idx];
}

void
tidbag_set(tidbag_t *tbag, size_t idx, task_id id)
{
    ASSERT(tbag);
    if (tbag->size == 0)
        return;

    ASSERT(tbag->tasks);
    ASSERT(id != NO_TASK);
    if (idx >= tbag->size)
        return;

    tbag->tasks[idx] = id;
}

void
tidbag_remove(tidbag_t *tbag, task_id id)
{
    ASSERT(tbag);
    ASSERT(tbag->size == 0 || tbag->tasks);

    if (tbag->size == 0)
        return;

    for (size_t i = 0; i < tbag->size; i++) {
        if (tbag->tasks[i] != id)
            continue;
        tbag->tasks[i] = tbag->tasks[--tbag->size];
        return;
    }
}

bool
tidbag_has(const tidbag_t *tbag, task_id id)
{
    ASSERT(tbag);

    for (size_t i = 0; i < tbag->size; i++)
        if (tbag->tasks[i] == id)
            return true;
    return false;
}

void
tidbag_expand(tidbag_t *tbag, size_t cap)
{
    ASSERT(tbag);
    if (cap == 0)
        cap = TIDBAG_INIT_SIZE;

    if (tbag->tasks == NULL) {
        tidbag_init_cap(tbag, cap);
        return;
    }
    if (tbag->size > cap) {
        return;
    }

    tidbag_t tmp = {0};
    tidbag_init_cap(&tmp, cap);
    sys_memcpy(tmp.tasks, tbag->tasks, tbag->size * sizeof(task_id));
    sys_free(tbag->tasks);

    tbag->capacity = tmp.capacity;
    tbag->tasks    = tmp.tasks;
}

void
tidbag_insert(tidbag_t *tbag, task_id id)
{
    ASSERT(tbag);
    ASSERT(tbag->size == 0 || tbag->tasks);

    if (tbag->size + 1 > tbag->capacity) {
        tidbag_expand(tbag, tbag->capacity * 2);
    }

    tbag->tasks[tbag->size++] = id;
}

void
tidbag_copy(tidbag_t *dst, const tidbag_t *src)
{
    ASSERT(dst);
    ASSERT(src);
    if (dst->capacity < src->capacity)
        tidbag_expand(dst, src->capacity);
    dst->size = src->size;
    sys_memcpy(dst->tasks, src->tasks, sizeof(task_id) * src->size);
}

void
tidbag_filter(tidbag_t *tbag, bool (*predicate)(task_id))
{
    ASSERT(tbag);
    ASSERT(predicate);
    for (size_t i = 0; i < tbag->size;) {
        if (predicate(tbag->tasks[i])) {
            i++;
            continue;
        }
        tbag->tasks[i] = tbag->tasks[--tbag->size];
    }
}

bool
tidbag_equals(const tidbag_t *tbag1, const tidbag_t *tbag2)
{
    ASSERT(tbag1);
    ASSERT(tbag2);

    if (tbag2->size != tbag1->size) {
        return false;
    }
    if (tbag2->size == 0 && tbag1->size == 0) {
        return true;
    }

    for (size_t i = 0; i < tbag2->size; i++) {
        if (!tidbag_has(tbag1, tbag2->tasks[i])) {
            return false;
        }
    }

    return true;
}

/*******************************************************************************
 * marshaling implementation
 ******************************************************************************/
size_t
tidbag_msize(const marshable_t *m)
{
    ASSERT(m);
    tidbag_t *tbag = (tidbag_t *)m;
    return sizeof(tidbag_t) + sizeof(task_id) * tbag->size;
}

void *
tidbag_marshal(const marshable_t *m, void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    char *b        = (char *)buf;
    tidbag_t *tbag = (tidbag_t *)m;

    sys_memcpy(b, tbag, sizeof(tidbag_t));
    b += sizeof(tidbag_t);

    size_t len = sizeof(task_id) * tbag->size;
    sys_memcpy(b, tbag->tasks, len);

    return b + len;
}


const void *
tidbag_unmarshal(marshable_t *m, const void *buf)
{
    ASSERT(m);
    ASSERT(buf);

    size_t off = offsetof(marshable_t, payload);
    sys_memcpy(m->payload, buf + off, m->alloc_size - off);
    char *b = (char *)buf + m->alloc_size;

    tidbag_t *tbag = (tidbag_t *)m;

    ASSERT(tbag->size <= tbag->capacity);
    size_t len = sizeof(task_id) * tbag->capacity;
    if (tbag->tasks != NULL)
        sys_free(tbag->tasks);
    tbag->tasks = (task_id *)sys_malloc(len);

    len = sizeof(task_id) * tbag->size;
    sys_memcpy(tbag->tasks, b, len);
    return b + len;
}

void
tidbag_print(const marshable_t *m)
{
    ASSERT(m);
    const tidbag_t *tbag = (const tidbag_t *)m;
    log_printf("[");
    for (size_t i = 0; i < tbag->size; i++) {
        if (i != 0)
            log_printf(", ");
        log_printf("%lu", tbag->tasks[i]);
    }
    log_printf("]\n");
}
