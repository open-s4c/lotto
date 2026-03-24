#include <signal.h>

#include <lotto/engine/sequencer.h>
#include <lotto/runtime/ingress_events.h>

static void
pthread_create_sigabrt_probe_handle_(const capture_point *cp, event_t *e)
{
    (void)e;
    if (cp->src_type != EVENT_TASK_CREATE) {
        return;
    }
    raise(SIGABRT);
}
REGISTER_SEQUENCER_HANDLER(pthread_create_sigabrt_probe_handle_)
