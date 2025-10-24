/*
 */
#ifndef LOTTO_MEMPOOL_H
#define LOTTO_MEMPOOL_H

#include <lotto/sys/memmgr_impl.h>

#if LOTTO_MEMMGR_USER
    #define MEMPOOL_SIZE MEMPOOL_USER_SIZE
#elif LOTTO_MEMMGR_RUNTIME
    #define MEMPOOL_SIZE MEMPOOL_RUNTIME_SIZE
#endif

#endif
