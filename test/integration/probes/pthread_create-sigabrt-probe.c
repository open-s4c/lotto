#include <signal.h>

#include <lotto/engine/sequencer.h>
#include <lotto/runtime/ingress_events.h>

static void
pthread_create_sigabrt_probe_handle_(const capture_point *cp, event_t *e)
{
    (void)e;
    if (cp->type_id != EVENT_TASK_CREATE) {
        return;
    }
    raise(SIGABRT);
}
ON_SEQUENCER_CAPTURE(pthread_create_sigabrt_probe_handle_)
