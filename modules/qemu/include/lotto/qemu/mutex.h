#ifndef QEMU_MUTEX_H
#define QEMU_MUTEX_H

#include <lotto/qemu/lotto_udf.h>

static inline void
lotto_lock_acquiring(void *addr)
{
    LOTTO_LOCK_ACQ_A64(addr);
}

static inline void
lotto_lock_released(void *addr)
{
    LOTTO_LOCK_REL_A64(addr);
}


#endif
