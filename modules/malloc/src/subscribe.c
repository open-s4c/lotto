#include <dice/chains/capture.h>
#include <dice/events/malloc.h>
#include <dice/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/sys/string.h>
#include <lotto/sys/memmgr_runtime.h>
#include <lotto/sys/memmgr_user.h>
#include <stdatomic.h>
#include <stdint.h>

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MALLOC, {
    struct malloc_event *ev = EVENT_PAYLOAD(event);
    ev->func = memmgr_user_alloc;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_FREE, {
    struct free_event *ev = EVENT_PAYLOAD(event);
    ev->func = memmgr_user_free;
})

void *_calloc(size_t n, size_t s)
{
    void *ptr = memmgr_user_alloc(n * s);
    if (ptr != NULL) {
        sys_memset(ptr, 0, n * s);
    }
    return ptr;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_CALLOC, {
    struct calloc_event *ev = EVENT_PAYLOAD(event);
    ev->func = _calloc;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_REALLOC, {
    struct realloc_event *ev = EVENT_PAYLOAD(event);
    ev->func = memmgr_user_realloc;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_ALIGNED_ALLOC, {
    struct aligned_alloc_event *ev = EVENT_PAYLOAD(event);
    ev->func = memmgr_user_aligned_alloc;
})

int _posix_memalign(void **memptr, size_t alignment, size_t size)
{
    void *ptr = memmgr_user_aligned_alloc(alignment, size);
    if (ptr == NULL)
        return -1;
    *memptr = ptr;
    return 0;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_POSIX_MEMALIGN, {
    struct posix_memalign_event *ev = EVENT_PAYLOAD(event);
    ev->func = _posix_memalign;
})
