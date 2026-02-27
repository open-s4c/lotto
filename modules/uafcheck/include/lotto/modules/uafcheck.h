#ifndef LOTTO_UAFCHECK_H
#define LOTTO_UAFCHECK_H

#include <unistd.h>

#include <vsync/queue/bounded_spsc.h>
#include <vsync/spinlock/caslock.h>

#undef ASSERT

typedef struct uaf_alloc {
    void *base;
    size_t size;
    uint64_t canary;
    char data[0];
} alloc_t;

typedef struct uafc {
    caslock_t lock;
    bounded_spsc_t protected;
    void *(*alloc)(size_t);
    void *(*aligned_alloc)(size_t, size_t);
    void (*free)(void *);
    void *(*realloc)(void *, size_t);
} uafc_t;

uafc_t *uafc_init(void *(*alloc)(size_t),
                  void *(*aligned_alloc)(size_t, size_t), void (*free)(void *),
                  void *(*realloc)(void *, size_t));
void uafc_fini(uafc_t *uc);
void *uafc_alloc(uafc_t *uc, size_t n);
void *uafc_aligned_alloc(uafc_t *uc, size_t alignment, size_t size);
void *uafc_realloc(uafc_t *uc, void *p, size_t n);
void uafc_free(uafc_t *uc, void *p);

#endif
