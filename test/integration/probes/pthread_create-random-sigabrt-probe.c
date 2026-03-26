#include <inttypes.h>
#include <signal.h>

#include <dice/log.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/ingress_events.h>

static void
pthread_create_random_sigabrt_probe_handle_(const capture_point *cp, event_t *e)
{
    int trigger;

    (void)e;
    if (cp->src_type != EVENT_TASK_CREATE) {
        return;
    }
    trigger = (int)prng_range(0, 2);
    log_printf("HANDLER_RANDOM: %" PRIu64 " %d\n", cp->id, trigger);
    if (trigger) {
        raise(SIGABRT);
    }
}
ON_SEQUENCER_CAPTURE(pthread_create_random_sigabrt_probe_handle_)
