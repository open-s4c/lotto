#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include <dice/chains/capture.h>
#include <dice/chains/intercept.h>
#include <dice/events/pthread.h>
#include <dice/events/thread.h>
#include <dice/interpose.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <lotto/base/arg.h>
#include <lotto/base/category.h>
#include <lotto/base/context.h>
#include <lotto/evec.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
#include <lotto/sys/logger.h>

static int
pthread_nop_zero_()
{
    return 0;
}
static int
pthread_nop_one_()
{
    return 1;
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_LOCK, {
    struct pthread_mutex_lock_event *ev = EVENT_PAYLOAD(event);
    (void)_lotto_mutex_acquire_named("pthread_mutex_lock", ev->mutex);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})


PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_TRYLOCK, {
    struct pthread_mutex_trylock_event *ev = EVENT_PAYLOAD(event);

    ev->func =
        _lotto_mutex_tryacquire_named("pthread_mutex_trylock", ev->mutex) == 0 ?
            pthread_nop_zero_ :
            pthread_nop_one_;
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_UNLOCK, {
    struct pthread_mutex_unlock_event *ev = EVENT_PAYLOAD(event);
    (void)_lotto_mutex_release_named("pthread_mutex_unlock", ev->mutex);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})

#if !defined(__APPLE__)
PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_MUTEX_TIMEDLOCK, {
    struct pthread_mutex_timedlock_event *ev = EVENT_PAYLOAD(event);
    (void)_lotto_mutex_acquire_named("pthread_mutex_timedlock", ev->mutex);
    ev->func = pthread_nop_zero_;
    return PS_OK;
})
#endif
