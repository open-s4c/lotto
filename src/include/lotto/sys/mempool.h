/*
 */
#ifndef LOTTO_SYS_MEMPOOL_H
#define LOTTO_SYS_MEMPOOL_H

#include <stddef.h>

#include <vsync/spinlock/caslock.h>

#define NSTACKS ((size_t)18)

typedef struct entry entry_t;

typedef struct alloc {
    size_t capacity;
    size_t next;
    char *memory;
} alloc_t;

typedef struct mempool {
    void (*free)(void *);
    caslock_t lock;
    size_t allocated;
    struct alloc pool;
    entry_t *stack[NSTACKS];
} mempool_t;

mempool_t *mempool_init(void *(*alloc)(size_t), void (*free)(void *),
                        size_t cap);
void mempool_init_static(mempool_t *mempool, void *pool, size_t cap);
void mempool_fini(mempool_t *mp);
void *mempool_alloc(mempool_t *mp, size_t n);
void *mempool_aligned_alloc(mempool_t *mp, size_t alignment, size_t size);
void *mempool_realloc(mempool_t *mp, void *p, size_t n);
void mempool_free(mempool_t *mp, void *p);

#endif
