/*
 */
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/memory.h>
#include <lotto/sys/mempool.h>
#include <lotto/sys/string.h>
#include <lotto/util/contract.h>

#define MEMPOOL_ALIGNMENT       8
#define MEMPOOL_CANARY          0xBEEFFEED
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
        log_errorf(
            "Could not allocate size %lu. Max bucket size %lu. i=%u "
            "NSTACKS=%lu\n",
            size, _sizes[NSTACKS - 1], i, NSTACKS);
    }
    return i;
}

mempool_t *
mempool_init(void *(*alloc)(size_t), void (*free)(void *), size_t cap)
{
    mempool_t *mp = alloc(sizeof(mempool_t));
    ASSERT(mp);
    mp->free      = free;
    mp->allocated = 0;
    sys_memset(&mp->stack, 0, sizeof(entry_t *) * NSTACKS);

    cap += MEMPOOL_ALIGNMENT - 1;
    mp->pool.memory = alloc(cap);
    mp->pool.memory -=
        ((uintptr_t)mp->pool.memory - METADATA_SIZE) % MEMPOOL_ALIGNMENT;
    ASSERT(mp->pool.memory);

    sys_memset(mp->pool.memory, 0, cap);
    mp->pool.capacity = cap;
    mp->pool.next =
        ((uintptr_t)mp->pool.memory - METADATA_SIZE) % MEMPOOL_ALIGNMENT;
    caslock_init(&mp->lock);
    return mp;
}

void
mempool_init_static(mempool_t *mempool, void *pool, size_t cap)
{
    ASSERT(mempool);
    *mempool = (mempool_t){
        .pool = (struct alloc){.memory = pool, .capacity = cap, .next = 0}};
    ASSERT(pool);
    caslock_init(&mempool->lock);
}

void
mempool_fini(mempool_t *mp)
{
    ASSERT(mp);
    if (mp->free == NULL) {
        log_infof(
            "mempool not provided with free() to deallocate memory: %lu\n",
            mp->allocated);
        return;
    }
    log_infof("mempool allocated memory on fini: %lu\n", mp->allocated);
    if (mp->pool.memory)
        mp->free(mp->pool.memory);
    mp->free(mp);
}

void *
mempool_alloc(mempool_t *mp, size_t n)
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
        ASSERT(mp->pool.capacity >= mp->pool.next + size && "mempool overflow");
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
    GET_CANARY(ret)    = MEMPOOL_CANARY;
    return GET_RETURN_PTR(e);
}

void *
mempool_aligned_alloc(mempool_t *mp, size_t alignment, size_t size)
{
    void *chunk  = mempool_alloc(mp, size + alignment - 1);
    uint64_t mod = (uintptr_t)chunk % alignment;
    if (mod == 0) {
        return chunk;
    }
    ASSERT(alignment > MEMPOOL_ALIGNMENT);
    entry_t *e         = GET_ENTRY_PTR(chunk);
    void *ret          = chunk + alignment - mod;
    GET_ENTRY_PTR(ret) = e;
    GET_CANARY(ret)    = MEMPOOL_CANARY;
    return ret;
}

void *
mempool_realloc(mempool_t *mp, void *p, size_t n)
{
    ASSERT(mp);
    if (p == NULL) {
        return mempool_alloc(mp, n);
    }
    ASSERT(GET_CANARY(p) == MEMPOOL_CANARY);
    ASSERT(IN_POOL(mp, p));
    entry_t *e = GET_ENTRY_PTR(p);
    if (e->size >= (char *)p - (char *)e + n) {
        return p;
    }
    void *ptr = mempool_alloc(mp, n);
    if (ptr == NULL) {
        mempool_free(mp, p);
        return ptr;
    }
    ASSERT(IN_POOL(mp, p));
    ASSERT(GET_CANARY(p) == MEMPOOL_CANARY);
    n = e->size - METADATA_SIZE < n + METADATA_SIZE ? e->size - METADATA_SIZE :
                                                      n;
    sys_memcpy(ptr, p, n);
    mempool_free(mp, p);
    return ptr;
}

void
mempool_free(mempool_t *mp, void *p)
{
    if (p == NULL) {
        return;
    }

    ASSERT(GET_CANARY(p) == MEMPOOL_CANARY);
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
