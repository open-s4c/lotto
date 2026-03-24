#if !defined(__APPLE__)
    #include <dlfcn.h>
#endif

#include <dice/chains/intercept.h>
#include <dice/interpose.h>
#include <dice/pubsub.h>
#include <lotto/engine/pubsub.h>
#include <lotto/modules/autocept/events.h>

LOTTO_ADVERTISE_TYPE(EVENT_AUTOCEPT_CALL)

void *
autocept_before(struct autocept_call_event *ev)
{
    metadata_t md = {0};

    PS_PUBLISH(INTERCEPT_BEFORE, EVENT_AUTOCEPT_CALL, ev, &md);

    if (ev->func == NULL) {
#if defined(__APPLE__)
        return NULL;
#else
        ev->func = real_sym(ev->name, 0);
#endif
    }

    return (void *)ev->func;
}

void
autocept_after(struct autocept_call_event *ev)
{
    metadata_t md = {0};
    PS_PUBLISH(INTERCEPT_AFTER, EVENT_AUTOCEPT_CALL, ev, &md);
}
