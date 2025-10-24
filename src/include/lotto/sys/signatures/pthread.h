/*
 */

#ifndef LOTTO_SIGNATURES_PTHREAD_H
#define LOTTO_SIGNATURES_PTHREAD_H

#include <pthread.h>

#include <lotto/sys/signatures/defaults_head.h>

typedef void (*pthread_destructor_f)(void *);

#define SYS_PTHREAD_KEY_CREATE                                                 \
    SYS_FUNC(PTHREAD, return,                                                  \
             SIG(int, pthread_key_create, pthread_key_t *, key,                \
                 pthread_destructor_f, destructor), )
#define SYS_PTHREAD_SETSPECIFIC                                                \
    SYS_FUNC(PTHREAD, return,                                                  \
             SIG(int, pthread_setspecific, pthread_key_t, key, const void *,   \
                 value), )

#define FOR_EACH_SYS_PTHREAD_WRAPPED                                           \
    SYS_PTHREAD_KEY_CREATE                                                     \
    SYS_PTHREAD_SETSPECIFIC

#define FOR_EACH_SYS_PTHREAD_CUSTOM

#define FOR_EACH_SYS_PTHREAD                                                   \
    FOR_EACH_SYS_PTHREAD_WRAPPED                                               \
    FOR_EACH_SYS_PTHREAD_CUSTOM

#endif // LOTTO_SIGNATURES_PTHREAD_H
