#include <dlfcn.h>
#include <pthread.h>
#include <stdbool.h>

#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/check.h>
#include <lotto/engine/pubsub.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/events.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/mediator.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stdlib.h>
typedef void (*fini_t)();
static void _intercept_resume(mediator_t *m, capture_point *cp);

PS_ADVERTISE_TYPE(EVENT_TASK_INIT)
PS_ADVERTISE_TYPE(EVENT_TASK_FINI)
PS_ADVERTISE_TYPE(EVENT_TASK_CREATE)
PS_ADVERTISE_TYPE(EVENT_TASK_JOIN)

static void
_intercept_resume(mediator_t *m, capture_point *cp)
{
    logger_debugf("[%lu] prepare to resume type=%s\n", m->id,
                  ps_type_str(cp->type_id));

    switch (mediator_resume(m, cp)) {
        case MEDIATOR_OK:
            break;
        case MEDIATOR_ABORT:
            lotto_exit(cp, REASON_ABORT);
            sys_abort();
            break;
        case MEDIATOR_SHUTDOWN:
            lotto_exit(cp, REASON_SHUTDOWN);
            sys_abort();
            break;
        default:
            logger_fatalf("unexpected mediator resume output");
            break;
    };
}

PS_SUBSCRIBE(CHAIN_INGRESS_EVENT, ANY_EVENT, {
    capture_point *cp = EVENT_PAYLOAD(cp);
    cp->chain_id      = chain;
    cp->type_id       = type;
    mediator_t *m     = mediator_get(md, true);

    if (cp->blocking) {
        logger_fatalf("blocking ingress-event type=%s func=%s chain=%u\n",
                      ps_type_str(cp->type_id), cp->func ? cp->func : "<null>",
                      (unsigned)cp->chain_id);
    }
    ASSERT(!cp->blocking && "events in this chain cannot block in user code");

    if (!mediator_capture(m, cp)) {
        _intercept_resume(m, cp);
    }
    return PS_STOP_CHAIN;
})
PS_SUBSCRIBE(CHAIN_INGRESS_BEFORE, ANY_EVENT, {
    capture_point *cp = EVENT_PAYLOAD(cp);
    cp->chain_id      = chain;
    cp->type_id       = type;
    mediator_t *m     = mediator_get(md, true);

    if (!mediator_capture(m, cp)) {
        ENSURE(!cp->blocking);
        _intercept_resume(m, cp);
    }
    return PS_STOP_CHAIN;
})
PS_SUBSCRIBE(CHAIN_INGRESS_AFTER, ANY_EVENT, {
    capture_point *cp = EVENT_PAYLOAD(cp);
    cp->chain_id      = chain;
    cp->type_id       = type;

    mediator_t *m = mediator_get(md, true);
    if (cp->blocking) {
        logger_debugf("[%lu] return from '%s'\n", m->id, cp->func);
        mediator_return(m, cp);
        _intercept_resume(m, cp);
    } else {
        // If not a blocking return, this is an individual event and has to be
        // handled so.
        bool block = mediator_capture(m, cp);
        ASSERT(!block);
        _intercept_resume(m, cp);
    }
    return PS_STOP_CHAIN;
})

// PS_SUBSCRIBE(CAPTURE_BEFORE, ANY_EVENT, { logger_printf("warning\n"); })


typedef struct fini_node {
    fini_t func;
    struct fini_node *next;
} fini_node;

static fini_node *_fini_head = NULL;
static fini_node *_fini_tail = NULL;

void
lotto_intercept_register_fini(fini_t func)
{
    fini_node *node = sys_malloc(sizeof(*node));
    ASSERT(node != NULL);
    *node = (fini_node){
        .func = func,
        .next = NULL,
    };

    if (_fini_tail != NULL) {
        _fini_tail->next = node;
    } else {
        _fini_head = node;
    }
    _fini_tail = node;
}

void
lotto_intercept_fini()
{
    while (_fini_head != NULL) {
        fini_node *node = _fini_head;
        _fini_head      = node->next;
        node->func();
        sys_free(node);
    }
    _fini_tail = NULL;
}
