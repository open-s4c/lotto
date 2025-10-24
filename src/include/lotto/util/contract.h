/*
 */
#ifndef LOTTO_CONTRACT_H
#define LOTTO_CONTRACT_H
/**
 * @file contract.h
 * @brief Macros to implement runtime contracts.
 *
 * Contracts are optimized out when CONTRACT_DISABLE is defined.
 */
#include <lotto/util/macros.h>

/**
 * @def CONTRACT_DISABLE
 * @brief Disables any code annotated with contract macros.
 *
 * CONTRACT_DISABLE is also defined if ASSERT_DISABLE is defined.
 */
#if defined(DOC)
    #define CONTRACT_DISABLE
#endif
#if defined(ASSERT_DISABLE) && !defined(CONTRACT_DISABLE)
    #define CONTRACT_DISABLE
#endif

/**
 * @def CONTRACT(...)
 * @brief Marks a piece of code as part of the correctness contract.
 *
 * When contracts are disabled, the whole code passed as argument is
 * omitted. Here is an usage example in the scope of a function.
 *
 * ```c
 * void foo(int val) {
 *    CONTRACT({
 *       // ensures val is monotonically increasing
 *       static int prev = 0;
 *       ASSERT(val >= prev);
 *       prev = val;
 *    })
 *
 *    // foo's implementation
 *    ...
 * }
 * ```
 */
#if !defined(CONTRACT_DISABLE)
    #define CONTRACT(...) __VA_ARGS__
#else
    #define CONTRACT(...)
#endif

/**
 * @def CONTRACT_INIT(...)
 * @brief Adds contract initialization function to the compilation unit.
 *
 * There can be at most one `CONTRACT_INIT` definition per compilation
 * unit. When contracts are disabled, the function is omitted.
 */
#if !defined(CONTRACT_DISABLE)
    #define CONTRACT_INIT(code)                                                \
        static LOTTO_CONSTRUCTOR void _lotto_contract_init(void)               \
        {                                                                      \
            code                                                               \
        }

#else
    #define CONTRACT_INIT(...)
#endif

/**
 * @def CONTRACT_SUBSCRIBE(topic, code)
 * @brief Subcribes topic with a callback code.
 *
 * Uses `PS_SUBSCRIBE` underneath. When contracts are disabled, the subscription
 * is omitted.
 */
#if !defined(CONTRACT_DISABLE)
    #define CONTRACT_SUBSCRIBE(topic, code) PS_SUBSCRIBE_INTERFACE(topic, code)
#else
    #define CONTRACT_SUBSCRIBE(...)
#endif

/**
 * @def CONTRACT_ASSSERT(cond)
 * @brief The same as `ASSERT` but emphasize the relation to contracts.
 *
 * When contracts are disabled, the assertion is omitted.
 */
#if !defined(CONTRACT_DISABLE)
    #define CONTRACT_ASSERT(cond) ASSERT(cond)
#else
    #define CONTRACT_ASSERT(cond) LOTTO_NOP
#endif

/**
 * @def CONTRACT_GHOST(fields)
 * @brief Defines a static ghost state for the contract with name `_ghost`.
 *
 * There can be at most one `CONTRACT_GHOST` definition per compilation
 * unit. When contracts are disabled, the _ghost struct is omitted.
 *
 * The ghost state is defined as a `struct`. Here is a usage example.
 *
 * ```c
 * CONTRACT_GHOST({
 *     int prev; // previous value of function foo
 * })
 *
 * void foo(int val) {
 *     CONTRACT({
 *       // ensures val is monotonically increasing
 *       ASSERT(val >= _ghost.prev);
 *       _ghost.prev = val;
 *     })
 *     // foo's implementation
 *     ...
 * }
 * ```
 */
#if !defined(CONTRACT_DISABLE)
    #define CONTRACT_GHOST(fields) static struct fields _ghost;
#else
    #define CONTRACT_GHOST(...)
#endif

#endif
