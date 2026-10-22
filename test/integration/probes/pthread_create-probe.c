#include <assert.h>
#include <inttypes.h>

#include <dice/log.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/runtime/events.h>
#include <lotto/sys/ensure.h>

static int handler_calls_;
static unsigned registration_order_;

/* Repeated registration macros keep their translation-unit declaration order.
 */
ON_REGISTRATION_PHASE({ ENSURE(++registration_order_ == 1); })
ON_REGISTRATION_PHASE({ ENSURE(++registration_order_ == 2); })

int
lotto_test_handler_count_impl(void)
{
    return handler_calls_;
}

static void
pthread_create_probe_handle_(const capture_point *cp, event_t *e)
{
    (void)e;
    ENSURE(registration_order_ == 2);
    if (cp->type_id != EVENT_TASK_CREATE) {
        return;
    }
    log_printf("HANDLER: %" PRIu64 "\n", cp->id);
    handler_calls_++;
}
ON_SEQUENCER_CAPTURE(pthread_create_probe_handle_)

LOTTO_MODULE_INIT()
