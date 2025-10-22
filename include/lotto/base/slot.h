#ifndef LOTTO_SLOT_H
#define LOTTO_SLOT_H

#define FOR_EACH_SLOT                                                          \
    /* blocking handlers */                                                    \
    GEN_SLOT(CREATION)                                                         \
    GEN_SLOT(BLOCKING)                                                         \
    GEN_SLOT(JOIN)                                                             \
    GEN_SLOT(MUTEX)                                                            \
    GEN_SLOT(EVEC)                                                             \
    GEN_SLOT(POLL)                                                             \
    GEN_SLOT(TIMEOUT)                                                          \
    GEN_SLOT(IMPASSE)                                                          \
                                                                               \
    /* hard filters */                                                         \
    GEN_SLOT(USER_FILTER)                                                      \
    GEN_SLOT(REGION_PREEMPTION)                                                \
    GEN_SLOT(REGION_FILTER)                                                    \
    GEN_SLOT(FILTERING)                                                        \
                                                                               \
    /* feature handlers */                                                     \
    GEN_SLOT(AVAILABLE)                                                        \
    GEN_SLOT(WATCHDOG)                                                         \
    GEN_SLOT(ICHPT)                                                            \
    GEN_SLOT(RACE)                                                             \
    GEN_SLOT(ATOMIC)                                                           \
    GEN_SLOT(YIELD)                                                            \
    GEN_SLOT(ADDRESS)                                                          \
    GEN_SLOT(DROP)                                                             \
    GEN_SLOT(TERMINATION)                                                      \
    GEN_SLOT(CAS)                                                              \
                                                                               \
    /* advanced handlers */                                                    \
    GEN_SLOT(CONSTRAINT_SATISFACTION)                                          \
    GEN_SLOT(RUSTY_ENGINE)                                                     \
    GEN_SLOT(RECONSTRUCT)                                                      \
    GEN_SLOT(PRIORITY)                                                         \
    GEN_SLOT(QEMU)                                                             \
    GEN_SLOT(CAPTURE_GROUP)                                                    \
    GEN_SLOT(TASK_VELOCITY)                                                    \
                                                                               \
    /* strategy handlers */                                                    \
    GEN_SLOT(POS)                                                              \
    GEN_SLOT(PCT)                                                              \
                                                                               \
    /* error detectors */                                                      \
    GEN_SLOT(DEADLOCK)                                                         \
    GEN_SLOT(ENFORCEMENT)                                                      \
                                                                               \
    /* other components */                                                     \
    GEN_SLOT(PRNG)                                                             \
    GEN_SLOT(CONFIG)                                                           \
    GEN_SLOT(CONTRACT)                                                         \
    GEN_SLOT(SEQUENCER)                                                        \
    GEN_SLOT(RECORDER)                                                         \
    GEN_SLOT(INACTIVITY_TIMEOUT)                                               \
    GEN_SLOT(CATMGR)                                                           \
                                                                               \
    GEN_SLOT(ANYSTALL)


#define GEN_SLOT(slot) SLOT_##slot,
typedef enum slot { FOR_EACH_SLOT SLOT_END_ } slot_t;
#undef GEN_SLOT

/**
 * Returns a string representation of the slot.
 */
const char *slot_str(slot_t slot);

/**
 * Register a new slot
 */
slot_t new_slot(const char *name);
#endif
