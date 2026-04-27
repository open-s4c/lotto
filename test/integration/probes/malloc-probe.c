#include <dice/chains/intercept.h>
#include <dice/events/malloc.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>

static int malloc_before_calls_;
static int malloc_after_calls_;

int
lotto_test_malloc_before_count(void)
{
    return malloc_before_calls_;
}

int
lotto_test_malloc_after_count(void)
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
