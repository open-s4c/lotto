/*
 */
#ifndef LOTTO_MEMPOOL_H
#define LOTTO_MEMPOOL_H

#include <lotto/sys/memmgr_impl.h>

#if LOTTO_MEMMGR_USER
    #define LOTTO_MEMPOOL_SIZE LOTTO_MEMPOOL_USER_SIZE
#elif LOTTO_MEMMGR_RUNTIME
    #define LOTTO_MEMPOOL_SIZE LOTTO_MEMPOOL_RUNTIME_SIZE
#endif

#endif
