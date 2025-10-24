/*
 */
#ifndef LOTTO_CREP_H
#define LOTTO_CREP_H
/*******************************************************************************
 * @file crep.h
 * @brief record/replay of libc interface and other standard libraries
 *
 * libcrep allows one to record and replay functions such as fread, poll,
 * writev, etc. The trace of calls is recorded per thread in files. If the file
 * already exists, the real functions are not called, but instead their
 * previously recorded return values are used. Return values can also be passed
 * via pointers as normal arguments to the function.
 *
 * To capture a new function the following pattern is used:
 *
 * 1. Declare the function with the standard signature, eg,
 * `int foo(stuff_t *buf)`.
 * 2. Find the pointer to the real function (using sys/real.h).
 * 3. Create a variable `callfun_t fun` structure with pointers to all return
 * values.
 * 4. Call crep_replay(). If it returns true, then we are replaying and `fun`
 * contains the recovered values that can be returned to the user.
 * 5. If `crep_replay()` returned false, then we call the real function and
 * subsequently record the results with `crep_record()`.
 *
 * Here is an example:
 *
 * ```c
 * size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
 * {
 *     REAL_CREP(size_t, fread, void *ptr, size_t size, size_t nmemb,
 *               FILE *stream);
 *     size_t ret;
 *
 *     callfun_t fun = {
 *         .retval  = &ret,
 *         .retsize = sizeof(size_t),
 *         .nparams = 1,
 *         .params  = {
 *                {.data = ptr, .size = size * nmemb},
 *                NULL
 *          }};
 *
 *     if (!crep_replay(&fun)) {
 *         ret = REAL(fread, ptr, size, nmemb, stream);
 *         crep_record(&fun);
 *     }
 *     return ret;
 * }
 * ```
 *
 * @note The requirement to be able to replay function calls properly is that
 * threads call the same functions in the same order when reexecuting. That
 * requirement should be fulfilled by Lotto.
 ******************************************************************************/
#include <stdbool.h>
#include <string.h>

#include <lotto/base/callrec.h>
#include <lotto/base/task_id.h>
#include <lotto/sys/real.h>
#include <lotto/util/prototypes.h>

typedef struct crep crep_t;

/**
 * Tries to replay a function call.
 *
 * @param fun function parameter descriptor
 * @returns true if replayed the function call, otherwise false.
 */
bool crep_replay(callfun_t *fun);

/**
 * Records are kept in thread-based files and automatically picked up during
 * replay.
 *
 * @param fun function parameter descriptor
 */
void crep_record(const callfun_t *fun);

/**
 * Creates a backup of crep files used for replay.
 */
void crep_backup_make(void);

/**
 * Creates a backup of crep files replacing files for replay.
 */
void crep_backup_restore(void);

/**
 * Deletes the backup.
 */
void crep_backup_clean(void);

/**
 * Truncates crep files by clock.
 *
 * @param clk clock starting from which records are deleted
 */
void crep_truncate(clk_t clk);


void crep_disable_(void) __attribute__((weak));
void crep_enable_(void) __attribute__((weak));

/**
 * Disables crep capturing (per thread), forcing all intercepted calls to be
 * simply forwarded.
 */
static inline void
crep_disable(void)
{
    if (crep_disable_)
        crep_disable_();
}

/**
 * Enables crep capturing (per thread), to perform record and/or replay.
 */
static inline void
crep_enable(void)
{
    if (crep_enable_)
        crep_enable_();
}

/**
 * Declares a static function pointer in the scope of a function and initialize
 * it looking for the implementation in LIBC.
 */
#define REAL_CREP(T, F, ...)                                                   \
    REAL_APPLY(DECL, T, F, ##__VA_ARGS__);                                     \
    if (REAL_APPLY(NAME, F) == NULL) {                                         \
        REAL_APPLY(NAME, F) = real_func(REAL_APPLY(STR, F), REAL_LIBC);        \
    }                                                                          \
    do {                                                                       \
    } while (0)

clk_t sequencer_get_clk() __attribute__((weak));

/**
 * Helps to wrap functions with libcrep.
 *
 * @param S the signature of the function (@see SIG)
 * @param M the mapping to the function parameter descriptor (@see MAP)
 *
 * Example:
 *
 * ```c
 * CREP_FUNC(SIG(FILE *, fopen, const char *, path, const char *, mode),
 *           ({
 *               .retval  = &ret,
 *               .retsize = sizeof(FILE *),
 *           }))
 * ```
 */
#define CREP_FUNC(S, M)                                                           \
    RET_TYPE(S) FUNC(S)(ARGS_TYPEVARS(S))                                         \
    {                                                                             \
        REAL_CREP(RET_TYPE(S), FUNC_##S, ARGS_TYPEVARS(S));                       \
        if (!CREP_RECORD_ENABLED) {                                               \
            /* log_fatalf("crep is recording when it must not\n"); */             \
            return REAL(FUNC(S), ARGS_VARS(S));                                   \
        }                                                                         \
        RET_TYPE(S) ret;                                                          \
        callfun_t fun   = {.retval = &ret,                                        \
                           .clk    = sequencer_get_clk ? sequencer_get_clk() : 0, \
                           M};                                                    \
        size_t name_len = strlen(__FUNCTION__);                                   \
        name_len        = name_len < MAX_NAME ? name_len : MAX_NAME;              \
        memcpy(&fun.name, __FUNCTION__, name_len);                                \
        fun.name[name_len] = 0;                                                   \
        if (!crep_replay(&fun)) {                                                 \
            ret = REAL(FUNC(S), ARGS_VARS(S));                                    \
            crep_record(&fun);                                                    \
        }                                                                         \
        return ret;                                                               \
    }

/**
 * @def MAP(...)
 *
 * Fills the content of a `callfun_t` object (@see callfun_t).
 */
#define MAP(...) __VA_ARGS__

/**
 * @def SIG(R, F, T1, V1, ...)
 *
 * Signature of a function `F`. The type of the return value if `R` and
 * the type of the parameter `Vi` is `Ti`. Note the comma separating parameter
 * type from value.
 */


#endif /* LOTTO_CREP_H */
