/*
 */
#ifndef LOTTO_ENSURE_H
#define LOTTO_ENSURE_H
/**
 * @file ensure.h
 * @brief Defines `ENSURE` as a permanent `ASSERT`..
 *
 */
#include <lotto/sys/abort.h>

/**
 * @def ENSURE_DISABLE
 * @brief Causes a compilation error if this header is included.
 */
#ifdef DOC
    #define ENSURE_DISABLE
#endif

#ifdef ENSURE_DISABLE
    #error "This file should not be included"
#endif

/**
 * @def ENSURE(cond)
 * @brief Ensures condition `cond` holds.
 *
 * ENSURE works just as ASSERT, but ENSURE is never removed in compile time.
 * In general, if a function is called in the assertion, we must use `ENSURE`
 * unless the function has no side effect or its side effect is irrelevant.
 */
#define ENSURE(cond)                                                           \
    do {                                                                       \
        if (!(cond))                                                           \
            sys_assert_fail(#cond, __FILE__, (unsigned int)__LINE__,           \
                            __FUNCTION__);                                     \
    } while (0)

#endif
