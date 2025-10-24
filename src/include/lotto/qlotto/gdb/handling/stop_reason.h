/*
 */

#ifndef LOTTO_GDB_STOP_REASON
#define LOTTO_GDB_STOP_REASON

#define FOR_EACH_STOP_REASON                                                   \
    GEN_STOP_REASON(watch)                                                     \
    GEN_STOP_REASON(rwatch)                                                    \
    GEN_STOP_REASON(awatch)                                                    \
    GEN_STOP_REASON(syscall_entry)                                             \
    GEN_STOP_REASON(syscall_return)                                            \
    GEN_STOP_REASON(library)                                                   \
    GEN_STOP_REASON(replaylog)                                                 \
    GEN_STOP_REASON(swbreak)                                                   \
    GEN_STOP_REASON(hwbreak)                                                   \
    GEN_STOP_REASON(fork)                                                      \
    GEN_STOP_REASON(vfork)                                                     \
    GEN_STOP_REASON(vforkdone)                                                 \
    GEN_STOP_REASON(exec)                                                      \
    GEN_STOP_REASON(clone)                                                     \
    GEN_STOP_REASON(create)

#define GEN_STOP_REASON(sr) STOP_REASON_##sr,

typedef enum stop_reason {
    STOP_REASON_NONE = 0,
    FOR_EACH_STOP_REASON STOP_REASON_END_
} stop_reason_t;

#undef GEN_STOP_REASON

/**
 * Returns a string representation of the udf trace name.
 */
const char *stop_reason_str(stop_reason_t t);

const char *stop_reason_str(stop_reason_t sr);

#endif // LOTTO_GDB_STOP_REASON
