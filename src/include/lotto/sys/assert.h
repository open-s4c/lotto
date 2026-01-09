/*
 */
#ifndef LOTTO_ASSERT_H
#define LOTTO_ASSERT_H
/**
 * @file assert.h
 * @brief Defines `ASSERT` macro.
 *
 * Use assertions to check assumptions about program. Assertions are only
 * checked if `ASSERT_DISABLE` is not set. To check for error conditions, use if
 * statement and `logger_fatalf`.
 */
#include <lotto/sys/abort.h>
#include <lotto/util/macros.h>
#include <lotto/util/unused.h>

/**
 * @def ASSERT_DISABLE
 * @brief Disables ASSERT macro replacing it with `do {} while(0)`.
 */
#ifdef DOC
    #define ASSERT_DISABLE
#endif

/**
 * @def ASSERT(cond)
 * @brief Asserts condition `cond` holds.
 *
 * Note that ASSERT is remove in compile time on optimized builds.
 */
#undef ASSERT
#ifdef ASSERT_DISABLE
    #define ASSERT(cond) (void)(cond)
#else
    #define ASSERT(cond)                                                       \
        do {                                                                   \
            if (!(cond))                                                       \
                sys_assert_fail(#cond, __FILE__, (unsigned int)__LINE__,       \
                                __FUNCTION__);                                 \
        } while (0)
#endif

#endif
