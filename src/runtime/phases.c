#include <dice/chains/intercept.h>
#include <dice/events/self.h>
#include <dice/module.h>
#include <dice/self.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/events.h>
#include <lotto/runtime/runtime.h>
#include <lotto/util/once.h>

static void
start_lotto_phases_(void)
{
    once({
        START_REGISTRATION_PHASE();
        START_INITIALIZATION_PHASE();

        lotto_runtime_bootstrap_begin();
        struct metadata *md = self_md();
        if (md != NULL)
            PS_PUBLISH(CAPTURE_EVENT, EVENT_SELF_INIT, NULL, md);
        else
            PS_PUBLISH(INTERCEPT_EVENT, EVENT_RUNTIME__NOP, NULL, NULL);
        lotto_runtime_bootstrap_end();
    });
}

static void __attribute__((constructor))
lotto_runtime_start_phases_(void)
{
    start_lotto_phases_();
}
