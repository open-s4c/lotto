#ifndef LOTTO_RSRC_DEADLOCK_H
#define LOTTO_RSRC_DEADLOCK_H

#include <stdbool.h>

void _lotto_rsrc_acquiring(void *addr) __attribute__((weak));
void _lotto_rsrc_released(void *addr) __attribute__((weak));

static inline void
lotto_rsrc_acquiring(void *addr)
{
    if (_lotto_rsrc_acquiring != NULL) {
        _lotto_rsrc_acquiring(addr);
    }
}

static inline bool
lotto_rsrc_tried_acquiring(void *addr, bool success)
{
    if (success) {
        lotto_rsrc_acquiring(addr);
    }
    return success;
}

static inline void
lotto_rsrc_released(void *addr)
{
    if (_lotto_rsrc_released != NULL) {
        _lotto_rsrc_released(addr);
    }
}

#endif /* LOTTO_RSRC_DEADLOCK_H */
