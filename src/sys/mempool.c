/*
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/mempool.h>
#include <lotto/sys/string.h>
#include <lotto/util/contract.h>

#define LOTTO_MEMPOOL_ALIGNMENT       8
#define LOTTO_MEMPOOL_CANARY          0xBEEFFEED
#define GET_ENTRY_PTR_STRUCT(p) ((entryptr_t *)(p)-1)
#define GET_CANARY(p)           GET_ENTRY_PTR_STRUCT(p)->canary
#define GET_ENTRY_PTR(p)        GET_ENTRY_PTR_STRUCT(p)->entry
#define GET_ENTRY(p)            (*GET_ENTRY_PTR(p))
#define GET_RETURN_PTR(e)       ((void *)(e) + METADATA_SIZE)

#define METADATA_SIZE (sizeof(entry_t) + sizeof(entryptr_t))
#define IN_POOL(mp, p)                                                         \
    ((char *)(p) >= (mp)->pool.memory &&                                       \
     (char *)(p) < (mp)->pool.memory + (mp)->pool.capacity)

typedef struct entry {
    struct entry *next;
    size_t size;
    char data[];
} entry_t;

typedef struct entryptr {
    entry_t *entry;
    uint64_t canary;
} entryptr_t;

static size_t _sizes[NSTACKS] = {32,
                                 128,
                                 256,
                                 512,
                                 1024,
                                 (size_t)2 * 1024,
                                 (size_t)4 * 1024,
                                 (size_t)8 * 1024,
                                 (size_t)16 * 1024,
                                 (size_t)32 * 1024,
                                 (size_t)64 * 1024,
                                 (size_t)128 * 1024,
                                 (size_t)256 * 1024,
                                 (size_t)512 * 1024,
                                 (size_t)1024 * 1024,
                                 (size_t)2 * 1024 * 1024,
                                 (size_t)4 * 1024 * 1024,
                                 (size_t)8 * 1024 * 1024};

unsigned int
_bucketize(size_t size)
{
    unsigned int i = 0;
    for (; i < NSTACKS && size > _sizes[i]; i++)
        ;
    if (i >= NSTACKS) {
        logger_errorf(
            "Could not allocate size %lu. Max bucket size %lu. i=%u "
            "NSTACKS=%lu\n",
            size, _sizes[NSTACKS - 1], i, NSTACKS);
    }
    return i;
}

lotto_mempool_t *
lotto_mempool_init(void *(*alloc)(size_t), void (*free)(void *), size_t cap)
{
    lotto_mempool_t *mp = alloc(sizeof(lotto_mempool_t));
    ASSERT(mp);
    mp->free      = free;
    mp->allocated = 0;
    sys_memset(&mp->stack, 0, sizeof(entry_t *) * NSTACKS);

    cap += LOTTO_MEMPOOL_ALIGNMENT - 1;
    mp->pool.memory = alloc(cap);
    mp->pool.memory -=
        ((uintptr_t)mp->pool.memory - METADATA_SIZE) % LOTTO_MEMPOOL_ALIGNMENT;
    ASSERT(mp->pool.memory);

    sys_memset(mp->pool.memory, 0, cap);
    mp->pool.capacity = cap;
    mp->pool.next =
        ((uintptr_t)mp->pool.memory - METADATA_SIZE) % LOTTO_MEMPOOL_ALIGNMENT;
    caslock_init(&mp->lock);
    return mp;
}

void
lotto_mempool_init_static(lotto_mempool_t *lotto_mempool, void *pool,
                          size_t cap)
{
    ASSERT(lotto_mempool);
    *lotto_mempool = (lotto_mempool_t){
        .pool = (struct alloc){.memory = pool, .capacity = cap, .next = 0}};
    ASSERT(pool);
    caslock_init(&lotto_mempool->lock);
}

void
lotto_mempool_fini(lotto_mempool_t *mp)
{
    ASSERT(mp);
    if (mp->free == NULL) {
        logger_infof(
            "lotto_mempool not provided with free() to deallocate memory: "
            "%lu\n",
            mp->allocated);
        return;
    }
    logger_infof("lotto_mempool allocated memory on fini: %lu\n",
                 mp->allocated);
    if (mp->pool.memory)
        mp->free(mp->pool.memory);
    mp->free(mp);
}

void *
lotto_mempool_alloc(lotto_mempool_t *mp, size_t n)
{
    ASSERT(mp);
    entry_t *e      = NULL;
    size_t size     = n + METADATA_SIZE;
    unsigned bucket = _bucketize(size);
    size            = _sizes[bucket];
    entry_t **stack = &mp->stack[bucket];
    ASSERT(stack);

    // Mempool is used from rogue thread, serialization is necessary
    caslock_acquire(&mp->lock);

    if (*stack) {
        e       = *stack;
        *stack  = e->next;
        e->next = NULL;
        mp->allocated += size;
    }


    if (e == NULL) {
        ASSERT(mp->pool.capacity >= mp->pool.next + size &&
               "lotto_mempool overflow");
        e       = (entry_t *)(mp->pool.memory + mp->pool.next);
        e->next = NULL;
        e->size = size;
        mp->pool.next += size;
        mp->allocated += size;
    }

    caslock_release(&mp->lock);

    if (e == NULL) {
        return NULL;
    }

    void *ret          = GET_RETURN_PTR(e);
    GET_ENTRY_PTR(ret) = e;
    GET_CANARY(ret)    = LOTTO_MEMPOOL_CANARY;
    return GET_RETURN_PTR(e);
}

void *
lotto_mempool_aligned_alloc(lotto_mempool_t *mp, size_t alignment, size_t size)
{
    void *chunk  = lotto_mempool_alloc(mp, size + alignment - 1);
    uint64_t mod = (uintptr_t)chunk % alignment;
    if (mod == 0) {
        return chunk;
    }
    ASSERT(alignment > LOTTO_MEMPOOL_ALIGNMENT);
    entry_t *e         = GET_ENTRY_PTR(chunk);
    void *ret          = chunk + alignment - mod;
    GET_ENTRY_PTR(ret) = e;
    GET_CANARY(ret)    = LOTTO_MEMPOOL_CANARY;
    return ret;
}

void *
lotto_mempool_realloc(lotto_mempool_t *mp, void *p, size_t n)
{
    ASSERT(mp);
    if (p == NULL) {
        return lotto_mempool_alloc(mp, n);
    }
    ASSERT(GET_CANARY(p) == LOTTO_MEMPOOL_CANARY);
    ASSERT(IN_POOL(mp, p));
    entry_t *e = GET_ENTRY_PTR(p);
    if (e->size >= (char *)p - (char *)e + n) {
        return p;
    }
    void *ptr = lotto_mempool_alloc(mp, n);
    if (ptr == NULL) {
        lotto_mempool_free(mp, p);
        return ptr;
    }
    ASSERT(IN_POOL(mp, p));
    ASSERT(GET_CANARY(p) == LOTTO_MEMPOOL_CANARY);
    n = e->size - METADATA_SIZE < n + METADATA_SIZE ? e->size - METADATA_SIZE :
                                                      n;
    sys_memcpy(ptr, p, n);
    lotto_mempool_free(mp, p);
    return ptr;
}

void
lotto_mempool_free(lotto_mempool_t *mp, void *p)
{
    if (p == NULL) {
        return;
    }

    ASSERT(GET_CANARY(p) == LOTTO_MEMPOOL_CANARY);
    ASSERT(mp);
    ASSERT(IN_POOL(mp, p));
    entry_t *e      = GET_ENTRY_PTR(p);
    size_t size     = e->size;
    unsigned bucket = _bucketize(size);
    size            = _sizes[bucket];
    entry_t **stack = &mp->stack[bucket];

    // Mempool is used from rogue thread, serialization is necessary
    caslock_acquire(&mp->lock);
    mp->allocated -= size;
    ASSERT(stack);
    e->next = *stack;
    *stack  = e;
    caslock_release(&mp->lock);
}
