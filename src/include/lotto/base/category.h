/*
 */
#ifndef LOTTO_CATEGORY_H
#define LOTTO_CATEGORY_H
/**
 * @file category.h
 * @brief The category of the calling context.
 *
 * Whenever a task calls the Lotto runtime, the context from which the call is
 * performed has a category.
 */

#define FOR_EACH_CATEGORY                                                      \
    GEN_CAT(NONE)                                                              \
    GEN_CAT(BEFORE_WRITE)                                                      \
    GEN_CAT(BEFORE_READ)                                                       \
    GEN_CAT(BEFORE_AWRITE)                                                     \
    GEN_CAT(BEFORE_AREAD)                                                      \
    GEN_CAT(BEFORE_XCHG)                                                       \
    GEN_CAT(BEFORE_RMW)                                                        \
    GEN_CAT(BEFORE_CMPXCHG)                                                    \
    GEN_CAT(BEFORE_FENCE)                                                      \
    GEN_CAT(AFTER_AWRITE)                                                      \
    GEN_CAT(AFTER_AREAD)                                                       \
    GEN_CAT(AFTER_XCHG)                                                        \
    GEN_CAT(AFTER_RMW)                                                         \
    GEN_CAT(AFTER_CMPXCHG_S)                                                   \
    GEN_CAT(AFTER_CMPXCHG_F)                                                   \
    GEN_CAT(AFTER_FENCE)                                                       \
    GEN_CAT(CALL)                                                              \
    GEN_CAT(TASK_BLOCK)                                                        \
    GEN_CAT(TASK_CREATE)                                                       \
    GEN_CAT(TASK_INIT)                                                         \
    GEN_CAT(TASK_FINI)                                                         \
    GEN_CAT(MUTEX_ACQUIRE)                                                     \
    GEN_CAT(MUTEX_TRYACQUIRE)                                                  \
    GEN_CAT(MUTEX_RELEASE)                                                     \
    GEN_CAT(RSRC_ACQUIRING)                                                    \
    GEN_CAT(RSRC_RELEASED)                                                     \
    GEN_CAT(SYS_YIELD)                                                         \
    GEN_CAT(USER_YIELD)                                                        \
    GEN_CAT(REGION_PREEMPTION)                                                 \
    GEN_CAT(FUNC_ENTRY)                                                        \
    GEN_CAT(FUNC_EXIT)                                                         \
    GEN_CAT(LOG_BEFORE)                                                        \
    GEN_CAT(LOG_AFTER)                                                         \
    GEN_CAT(REGION_IN)                                                         \
    GEN_CAT(REGION_OUT)                                                        \
    GEN_CAT(EVEC_PREPARE)                                                      \
    GEN_CAT(EVEC_WAIT)                                                         \
    GEN_CAT(EVEC_TIMED_WAIT)                                                   \
    GEN_CAT(EVEC_CANCEL)                                                       \
    GEN_CAT(EVEC_WAKE)                                                         \
    GEN_CAT(EVEC_MOVE)                                                         \
    GEN_CAT(ENFORCE)                                                           \
    GEN_CAT(POLL)                                                              \
    GEN_CAT(TASK_VELOCITY)                                                     \
    GEN_CAT(KEY_CREATE)                                                        \
    GEN_CAT(KEY_DELETE)                                                        \
    GEN_CAT(SET_SPECIFIC)                                                      \
    GEN_CAT(JOIN)                                                              \
    GEN_CAT(DETACH)                                                            \
    GEN_CAT(EXIT)


#define GEN_CAT(cat) CAT_##cat,
typedef enum base_category { FOR_EACH_CATEGORY CAT_END_ } category_t;
#undef GEN_CAT

#define CAT_BLOCK(x)                                                           \
    ((x) == CAT_CALL || (x) == CAT_TASK_BLOCK || (x) == CAT_TASK_CREATE)

#define CAT_SLACK(x) ((x) == CAT_CALL || (x) == CAT_TASK_BLOCK)

#define CAT_WAIT(x)                                                            \
    ((x) == CAT_EVEC_WAIT || (x) == CAT_EVEC_TIMED_WAIT ||                     \
     (x) == CAT_MUTEX_ACQUIRE || (x) == CAT_POLL || (x) == CAT_JOIN)

/**
 * Returns a string representation of the category `cat`.
 */
const char *base_category_str(category_t cat);

#endif
