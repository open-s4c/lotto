/*
 */
#ifndef LOTTO_REASON_H
#define LOTTO_REASON_H

#include <stdbool.h>

#define FOR_EACH_REASON                                                        \
    GEN_REASON(UNKNOWN)                                                        \
    GEN_REASON(DETERMINISTIC)                                                  \
    GEN_REASON(NONDETERMINISTIC)                                               \
    GEN_REASON(CALL)                                                           \
    GEN_REASON(WATCHDOG)                                                       \
    GEN_REASON(SYS_YIELD)                                                      \
    GEN_REASON(USER_YIELD)                                                     \
    GEN_REASON(USER_ORDER)                                                     \
    GEN_REASON(ASSERT_FAIL)                                                    \
    GEN_REASON(RSRC_DEADLOCK)                                                  \
    GEN_REASON(SEGFAULT)                                                       \
    GEN_REASON(SIGINT)                                                         \
    GEN_REASON(SIGABRT)                                                        \
    GEN_REASON(SIGTERM)                                                        \
    GEN_REASON(SIGKILL)                                                        \
    GEN_REASON(SUCCESS)                                                        \
    GEN_REASON(IGNORE)                                                         \
    GEN_REASON(ABORT)                                                          \
    GEN_REASON(SHUTDOWN)                                                       \
    GEN_REASON(RUNTIME_SEGFAULT)                                               \
    GEN_REASON(RUNTIME_SIGINT)                                                 \
    GEN_REASON(RUNTIME_SIGABRT)                                                \
    GEN_REASON(RUNTIME_SIGTERM)                                                \
    GEN_REASON(RUNTIME_SIGKILL)                                                \
    GEN_REASON(IMPASSE)

#define GEN_REASON(reason) REASON_##reason,
typedef enum reason { FOR_EACH_REASON REASON_END_ } reason_t;
#undef GEN_REASON

#define REASON_RUNTIME(r)                                                      \
    ((r) == REASON_RUNTIME_SEGFAULT || (r) == REASON_RUNTIME_SIGINT ||         \
     (r) == REASON_RUNTIME_SIGABRT || (r) == REASON_RUNTIME_SIGTERM ||         \
     (r) == REASON_RUNTIME_SIGKILL)

#define IS_REASON_SHUTDOWN(r)                                                  \
    ((r) == REASON_SHUTDOWN || (r) == REASON_SUCCESS || (r) == REASON_IGNORE)

#define IS_REASON_ABORT(r)                                                     \
    ((r) == REASON_ASSERT_FAIL || (r) == REASON_RSRC_DEADLOCK ||               \
     (r) == REASON_SEGFAULT || (r) == REASON_SIGINT ||                         \
     (r) == REASON_SIGABRT || (r) == REASON_ABORT ||                           \
     (r) == REASON_RUNTIME_SEGFAULT || (r) == REASON_RUNTIME_SIGINT ||         \
     (r) == REASON_RUNTIME_SIGABRT || (r) == REASON_IMPASSE ||                 \
     (r) == REASON_SIGTERM || (r) == REASON_RUNTIME_SIGTERM ||                 \
     (r) == REASON_SIGKILL || (r) == REASON_RUNTIME_SIGKILL)

#define IS_REASON_TERMINATE(r) (IS_REASON_SHUTDOWN(r) || IS_REASON_ABORT(r))

#define IS_REASON_RECORD_FINAL(r)                                              \
    ((r) == REASON_SUCCESS || (r) == REASON_ASSERT_FAIL ||                     \
     (r) == REASON_SHUTDOWN || (r) == REASON_ABORT)

const char *reason_str(reason_t r);

bool reason_is_runtime(reason_t r);
bool reason_is_shutdown(reason_t r);
bool reason_is_abort(reason_t r);
bool reason_is_terminate(reason_t r);
bool reason_is_record_final(reason_t r);

#endif
