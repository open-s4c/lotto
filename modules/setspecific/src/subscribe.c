#include <pthread.h>
#include <stdlib.h>

#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/category.h>
#include <lotto/engine/pubsub.h>
#include <lotto/evec.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/ingress.h>
#include <lotto/runtime/ingress_events.h>
#include <lotto/states/handlers/deadlock.h>
#include <lotto/states/handlers/mutex.h>
#include <lotto/sys/abort.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/real.h>
#include <lotto/sys/stdlib.h>
#include <lotto/unsafe/disable.h>
#include <lotto/unsafe/rogue.h>
#include <lotto/yield.h>
#include <vsync/atomic/core.h>
#include <vsync/atomic/dispatch.h>
#include <vsync/vtypes.h>

typedef struct key_create_event {
    pthread_key_t *key;
    void (*destructor)(void *);
} key_create_event_t;

typedef struct key_delete_event {
    pthread_key_t key;
} key_delete_event_t;

typedef struct set_specific_event {
    pthread_key_t key;
    const void *value;
} set_specific_event_t;

PS_ADVERTISE_TYPE(EVENT_KEY_CREATE)
PS_ADVERTISE_TYPE(EVENT_KEY_DELETE)
PS_ADVERTISE_TYPE(EVENT_SET_SPECIFIC)

static void
intercept_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    key_create_event_t ev = {.key = key, .destructor = destructor};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_KEY_CREATE, &ev, 0);
}

static void
intercept_key_delete(pthread_key_t key)
{
    key_delete_event_t ev = {.key = key};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_KEY_DELETE, &ev, 0);
}

static void
intercept_set_specific(pthread_key_t key, const void *value)
{
    set_specific_event_t ev = {.key = key, .value = value};
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_SET_SPECIFIC, &ev, 0);
}

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    REAL_INIT(int, pthread_key_create, pthread_key_t *key,
              void (*destructor)(void *));
    int ret = REAL(pthread_key_create, key, NULL);
    if (ret == 0) {
        intercept_key_create(key, destructor);
    }
    ASSERT(ret == 0);
    return ret;
}

int
pthread_key_delete(pthread_key_t key)
{
    intercept_key_delete(key);
    REAL_INIT(int, pthread_key_delete, pthread_key_t key);
    return REAL(pthread_key_delete, key);
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
    intercept_set_specific(key, value);
    REAL_INIT(int, pthread_setspecific, pthread_key_t key, const void *value);
    return REAL(pthread_setspecific, key, value);
}

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_KEY_CREATE, {
    key_create_event_t *ev = EVENT_PAYLOAD(event);
    context_origin ctx = *ctx_origin(.self = md, .func = "pthread_key_create");
    capture_key_create_event ce = {
        .key        = ev->key,
        .destructor = ev->destructor,
    };
    capture_point cp = {.src_type = EVENT_KEY_CREATE, .key_create = &ce};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_KEY_CREATE, &cp, (metadata_t *)&ctx);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_KEY_DELETE, {
    key_delete_event_t *ev = EVENT_PAYLOAD(event);
    context_origin ctx = *ctx_origin(.self = md, .func = "pthread_key_delete");
    capture_key_delete_event ce = {.key = ev->key};
    capture_point cp = {.src_type = EVENT_KEY_DELETE, .key_delete = &ce};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_KEY_DELETE, &cp, (metadata_t *)&ctx);
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_EVENT, EVENT_SET_SPECIFIC, {
    set_specific_event_t *ev = EVENT_PAYLOAD(event);
    context_origin ctx = *ctx_origin(.self = md, .func = "pthread_setspecific");
    capture_set_specific_event ce = {
        .key   = ev->key,
        .value = ev->value,
    };
    capture_point cp = {.src_type = EVENT_SET_SPECIFIC, .set_specific = &ce};
    PS_PUBLISH(CHAIN_INGRESS_EVENT, EVENT_SET_SPECIFIC, &cp, (metadata_t *)&ctx);
    return PS_OK;
})
