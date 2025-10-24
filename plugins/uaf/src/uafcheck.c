#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/mman.h>
#include <vsync/queue/bounded_spsc.h>
#include <vsync/queue/internal/bounded_ret.h>
#undef ASSERT

#include <uafcheck.h>

#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/string.h>

#define UAFC_CANARY 0xBEEFFEED
#define UAFC_MAXP   16
#define UAFC_PROB   0.5

#define METADATA_SIZE sizeof(alloc_t)

static uintptr_t _pagesize;
static uintptr_t _pagemask;

uafc_t *
uafc_init(void *(*alloc)(size_t), void *(*aligned_alloc)(size_t, size_t),
          void (*free)(void *), void *(*realloc)(void *, size_t))
{
    if (_pagesize == 0) {
        long pagesize = sysconf(_SC_PAGE_SIZE);
        ASSERT(pagesize != -1);

        _pagesize = (uintptr_t)pagesize;
        _pagemask = ~(pagesize - 1);
    }

    uafc_t *uc = alloc(sizeof(uafc_t));
    ASSERT(uc);

    *uc = (uafc_t){.alloc         = alloc,
                   .aligned_alloc = aligned_alloc,
                   .free          = free,
                   .realloc       = realloc};

    size_t len = sizeof(alloc_t *) * UAFC_MAXP;
    void *buf  = alloc(len);
    ASSERT(buf);

    bounded_spsc_init(&uc->protected, buf, UAFC_MAXP);
    caslock_init(&uc->lock);

    return uc;
}

void *
uafc_alloc(uafc_t *uc, size_t n)
{
    ASSERT(uc);

    // sample probability
    int p = (int)(rand() * 1.0 / RAND_MAX);
    if (p > UAFC_PROB)
        return uc->alloc(n);

    size_t size    = n + METADATA_SIZE + 2 * _pagesize;
    uintptr_t base = (uintptr_t)uc->alloc(size);
    ASSERT(base);

    // allocation
    // base ptr
    //   |                        |
    //   +------ next page -------+
    //   | alloc_t                |
    //   +------------------------+
    //   | user data              |
    //   | ...                    |

    uintptr_t next = (base + _pagesize + METADATA_SIZE) & _pagemask;
    alloc_t *a     = (alloc_t *)next - 1;
    ASSERT((uintptr_t)a >= base);
    ASSERT(next + n <= base + size);
    a->canary = UAFC_CANARY;
    a->base   = (void *)base;
    a->size   = n;

    return a->data;
}

static bool
_uafc_drop(uafc_t *uc)
{
    void *ptr = NULL;
    if (bounded_spsc_deq(&uc->protected, &ptr) == QUEUE_BOUNDED_EMPTY)
        return false;

    int ret = mprotect(ptr + METADATA_SIZE, _pagesize,
                       PROT_READ | PROT_EXEC | PROT_WRITE);
    if (ret != 0)
        log_fatalf("could not mprotect page\n");

    alloc_t *a = (alloc_t *)ptr;
    a->canary  = ~UAFC_CANARY;
    uc->free(a->base);

    return true;
}

void
uafc_free(uafc_t *uc, void *p)
{
    if (p == NULL) {
        return;
    }
    ASSERT(uc);
    alloc_t *a = (alloc_t *)p - 1;

    // if not alloc_t wouldn't be aligned or a is aligned but canary dead,
    // then p is not uafc allocated
    if (((uintptr_t)p & _pagemask) != (uintptr_t)p ||
        (a->canary != UAFC_CANARY)) {
        uc->free(p);
        return;
    }

    // probe double free
    volatile char c = *(char *)p;
    (void)c;

    // protect page and save
    int ret = mprotect(p, _pagesize, PROT_NONE);
    if (ret != 0)
        log_fatalf("could not protect page\n");

    ASSERT(a->canary == UAFC_CANARY);

    caslock_acquire(&uc->lock);
    if (bounded_spsc_enq(&uc->protected, a) == QUEUE_BOUNDED_FULL) {
        bool ok = _uafc_drop(uc);
        ASSERT(ok);
        int r = bounded_spsc_enq(&uc->protected, a);
        ASSERT(r == QUEUE_BOUNDED_OK);
    }
    caslock_release(&uc->lock);
}

void *
uafc_aligned_alloc(uafc_t *uc, size_t alignment, size_t size)
{
    ASSERT(alignment <= _pagesize &&
           "alignment greater than the pagesize not implemented");
    return uafc_alloc(uc, size);
}

void *
uafc_realloc(uafc_t *uc, void *p, size_t n)
{
    if (p == NULL) {
        return uafc_alloc(uc, n);
    }
    ASSERT(uc);
    alloc_t *a = (alloc_t *)p - 1;

    // if not alloc_t wouldn't be aligned or a is aligned but canary dead,
    // then p is not uafc allocated
    if (p == NULL || ((uintptr_t)p & _pagemask) != (uintptr_t)p ||
        (a->canary != UAFC_CANARY))
        return uc->realloc(p, n);

    void *np = uafc_alloc(uc, n);
    if (!np || !p) {
        return np;
    }

    n = a->size < n ? a->size : n;
    sys_memcpy(np, p, n);
    uafc_free(uc, p);

    return np;
}

void
uafc_fini(uafc_t *uc)
{
    ASSERT(uc);

    caslock_acquire(&uc->lock);
    while (_uafc_drop(uc))
        ;
    caslock_release(&uc->lock);
    uc->free(uc->protected.buf);
    uc->free(uc);
}
