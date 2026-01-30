#include <pthread.h>
#include <stdlib.h>

#include <lotto/base/category.h>
#include <lotto/evec.h>
#include <lotto/mutex.h>
#include <lotto/rsrc_deadlock.h>
#include <lotto/runtime/intercept.h>
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

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
    REAL_INIT(int, pthread_key_create, pthread_key_t *key,
              void (*destructor)(void *));
    int ret = REAL(pthread_key_create, key, NULL);
    if (ret == 0) {
        intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_KEY_CREATE,
                              .args = {arg_ptr(key), arg_ptr(destructor)}));
    }
    ASSERT(ret == 0);
    return ret;
}

int
pthread_key_delete(pthread_key_t key)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_KEY_DELETE,
                          .args = {arg_ptr(&key)}));
    REAL_INIT(int, pthread_key_delete, pthread_key_t key);
    return REAL(pthread_key_delete, key);
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
    intercept_capture(ctx(.func = __FUNCTION__, .cat = CAT_SET_SPECIFIC,
                          .args = {arg_ptr(&key), arg_ptr(value)}));
    REAL_INIT(int, pthread_setspecific, pthread_key_t key, const void *value);
    return REAL(pthread_setspecific, key, value);
}
