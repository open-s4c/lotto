/*
 */
#ifndef LOTTO_UNSAFE_ROGUE_H
#define LOTTO_UNSAFE_ROGUE_H

/**
 * Define a rogue region.
 */
#define lotto_rogue(...)                                                       \
    {                                                                          \
        char ghost __attribute__((__cleanup__(lotto_region_rogue_cleanup)));   \
        (void)ghost;                                                           \
        lotto_region_rogue_enter();                                            \
        __VA_ARGS__;                                                           \
    }

void _lotto_region_rogue_enter(void) __attribute__((weak));
void _lotto_region_rogue_leave(void) __attribute__((weak));

/**
 * Enter the rogue region.
 */
static inline void
lotto_region_rogue_enter(void)
{
    if (_lotto_region_rogue_enter != NULL) {
        _lotto_region_rogue_enter();
    }
}
/**
 * Leave the rogue region.
 */
static inline void
lotto_region_rogue_leave(void)
{
    if (_lotto_region_rogue_leave != NULL) {
        _lotto_region_rogue_leave();
    }
}

static inline void __attribute__((unused))
lotto_region_rogue_cleanup(void *ghost)
{
    (void)ghost;
    if (_lotto_region_rogue_leave != NULL) {
        _lotto_region_rogue_leave();
    }
}

#endif
