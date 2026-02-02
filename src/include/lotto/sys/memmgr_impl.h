#ifndef LOTTO_MEMMGR_IMPL_H
#define LOTTO_MEMMGR_IMPL_H

#include <lotto/util/macros.h>

#if defined(LOTTO_MEMMGR_USER) && defined(LOTTO_MEMMGR_RUNTIME)
    #error Either LOTTO_MEMMGR_USER or LOTTO_MEMMGR_RUNTIME must be defined
#elif LOTTO_MEMMGR_USER
    #include <lotto/sys/memmgr_user.h>
    #define MEMMGR_MODE user
#elif LOTTO_MEMMGR_RUNTIME
    #include <lotto/sys/memmgr_runtime.h>
    #define MEMMGR_MODE runtime
#else
    #error Either LOTTO_MEMMGR_USER or LOTTO_MEMMGR_RUNTIME must be defined
#endif

#define MEMMGR_PREFIX CONCAT(memmgr_, MEMMGR_MODE)

#define memmgr_init               CONCAT(MEMMGR_PREFIX, _init)
#define memmgr_alloc              CONCAT(MEMMGR_PREFIX, _alloc)
#define memmgr_alloc_name         XSTR(memmgr_alloc)
#define memmgr_aligned_alloc      CONCAT(MEMMGR_PREFIX, _aligned_alloc)
#define memmgr_aligned_alloc_name XSTR(memmgr_aligned_alloc)
#define memmgr_realloc            CONCAT(MEMMGR_PREFIX, _realloc)
#define memmgr_realloc_name       XSTR(memmgr_realloc)
#define memmgr_free               CONCAT(MEMMGR_PREFIX, _free)
#define memmgr_free_name          XSTR(memmgr_free)
#define memmgr_fini               CONCAT(MEMMGR_PREFIX, _fini)

#endif
