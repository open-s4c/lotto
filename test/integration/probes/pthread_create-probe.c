#include <inttypes.h>

#include <dice/log.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/ingress_events.h>

static int handler_calls_;

int
lotto_test_handler_count(void)
{
    return handler_calls_;
}

static void
pthread_create_probe_handle_(const capture_point *cp, event_t *e)
{
    (void)e;
    if (cp->src_type != EVENT_TASK_CREATE) {
        return;
    }
    log_printf("HANDLER: %" PRIu64 "\n", cp->id);
    handler_calls_++;
}
ON_SEQUENCER_CAPTURE(pthread_create_probe_handle_)
