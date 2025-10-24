/*
 */
#ifndef LOTTO_CHECK_H
#define LOTTO_CHECK_H
/*******************************************************************************
 * @file check.h
 * @brief Supporting functions for Lotto's users.
 ******************************************************************************/
#include <stdbool.h>
#include <stddef.h>

bool _lotto_loaded(void) __attribute__((weak));

/**
 * Returns true if Lotto is loaded, eg, with LD_PRELOAD.
 */
static inline bool
lotto_loaded(void)
{
    return _lotto_loaded != NULL && _lotto_loaded();
}

#endif
