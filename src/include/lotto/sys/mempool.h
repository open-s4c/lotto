/*
 */
#ifndef LOTTO_SYS_LOTTO_MEMPOOL_H
#define LOTTO_SYS_LOTTO_MEMPOOL_H

#include <stddef.h>

#include <vsync/spinlock/caslock.h>

#define NSTACKS ((size_t)18)

typedef struct entry entry_t;

typedef struct alloc {
    size_t capacity;
    size_t next;
    char *memory;
} alloc_t;

typedef struct lotto_mempool {
    void (*free)(void *);
    caslock_t lock;
    size_t allocated;
    struct alloc pool;
    entry_t *stack[NSTACKS];
} lotto_mempool_t;

lotto_mempool_t *lotto_mempool_init(void *(*alloc)(size_t), void (*free)(void *),
                        size_t cap);
void lotto_mempool_init_static(lotto_mempool_t *lotto_mempool, void *pool, size_t cap);
void lotto_mempool_fini(lotto_mempool_t *mp);
void *lotto_mempool_alloc(lotto_mempool_t *mp, size_t n);
void *lotto_mempool_aligned_alloc(lotto_mempool_t *mp, size_t alignment, size_t size);
void *lotto_mempool_realloc(lotto_mempool_t *mp, void *p, size_t n);
void lotto_mempool_free(lotto_mempool_t *mp, void *p);

#endif
