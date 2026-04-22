#include <dice/events/malloc.h>
#include <dice/interpose.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>

enum {
    MALLOC_SEGFAULT_SIZE = 65521,
};

static int malloc_before_calls_;
static int malloc_after_calls_;

int
lotto_test_malloc_segfault_before_count(void)
{
    return malloc_before_calls_;
}

int
lotto_test_malloc_segfault_after_count(void)
{
    return malloc_after_calls_;
}

static void
handler_(const capture_point *cp, event_t *e)
{
    (void)e;
    if (cp->type_id != EVENT_MALLOC) {
        return;
    }
    if (cp->chain_id == CHAIN_INGRESS_BEFORE) {
        malloc_before_calls_++;
        return;
    }
    if (cp->chain_id == CHAIN_INGRESS_AFTER) {
        malloc_after_calls_++;
    }
}
ON_SEQUENCER_CAPTURE(handler_)

INTERPOSE(void *, malloc, size_t size)
{
    if (size == MALLOC_SEGFAULT_SIZE) {
        *(volatile int *)0 = 1;
    }
    return REAL(malloc, size);
}
