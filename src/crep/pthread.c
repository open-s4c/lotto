#include <crep.h>
#include <pthread.h>
#include <stdio.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE
#define LOGGER_BLOCK  LOGGER_CUR_BLOCK
#include <lotto/base/callrec.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/real.h>

#ifndef QLOTTO_ENABLED
int
pthread_mutex_lock(pthread_mutex_t *m)
{
    REAL_CREP(int, pthread_mutex_lock, pthread_mutex_t *m);

    int ret;
    callfun_t fun = {
        .retval  = &ret,
        .retsize = sizeof(int),
    };

    if (!crep_replay(&fun)) {
        ret = REAL(pthread_mutex_lock, m);
        crep_record(&fun);
    }

    return ret;
}

int
pthread_mutex_trylock(pthread_mutex_t *m)
{
    REAL_CREP(int, pthread_mutex_trylock, pthread_mutex_t *m);

    int ret;
    callfun_t fun = {
        .retval  = &ret,
        .retsize = sizeof(int),
    };

    if (!crep_replay(&fun)) {
        ret = REAL(pthread_mutex_trylock, m);
        crep_record(&fun);
    }

    return ret;
}

int
pthread_mutex_unlock(pthread_mutex_t *m)
{
    REAL_CREP(int, pthread_mutex_unlock, pthread_mutex_t *m);

    int ret;
    callfun_t fun = {
        .retval  = &ret,
        .retsize = sizeof(int),
    };

    if (!crep_replay(&fun)) {
        ret = REAL(pthread_mutex_unlock, m);
        crep_record(&fun);
    }
    return 1;
}

#endif
