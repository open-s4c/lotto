/**
 * @file module_events.h
 * @brief Shared normalized module event ids.
 */
#ifndef LOTTO_RUNTIME_MODULE_EVENTS_H
#define LOTTO_RUNTIME_MODULE_EVENTS_H

#include <dice/types.h>

#ifndef EVENT_MUTEX_ACQUIRE
    #define EVENT_MUTEX_ACQUIRE    142
    #define EVENT_MUTEX_TRYACQUIRE 143
    #define EVENT_MUTEX_RELEASE    144
#endif
#ifndef EVENT_EVEC_PREPARE
    #define EVENT_EVEC_PREPARE    145
    #define EVENT_EVEC_WAIT       146
    #define EVENT_EVEC_TIMED_WAIT 147
    #define EVENT_EVEC_CANCEL     148
    #define EVENT_EVEC_WAKE       149
    #define EVENT_EVEC_MOVE       150
#endif
#ifndef EVENT_RSRC_ACQUIRING
    #define EVENT_RSRC_ACQUIRING 151
    #define EVENT_RSRC_RELEASED  152
#endif
#ifndef EVENT_TASK_VELOCITY
    #define EVENT_TASK_VELOCITY 154
#endif
#ifndef EVENT_REGION_PREEMPTION
    #define EVENT_REGION_PREEMPTION 155
#endif
#ifndef EVENT_ORDER
    #define EVENT_ORDER 156
#endif
#ifndef EVENT_FORK_EXECVE
    #define EVENT_FORK_EXECVE 157
#endif
#ifndef EVENT_AWAIT
    #define EVENT_AWAIT      159
    #define EVENT_SPIN_START 160
    #define EVENT_SPIN_END   161
#endif
#ifndef EVENT_TIME_YIELD
    #define EVENT_TIME_YIELD 162
#endif
#ifndef EVENT_POLL
    #define EVENT_POLL 164
#endif
#ifndef EVENT_SCHED_YIELD
    #define EVENT_SCHED_YIELD 169
#endif
#ifndef EVENT_CXA_GUARD_CALL
    #define EVENT_CXA_GUARD_CALL 175
#endif
#ifndef EVENT_TASK_JOIN
    #define EVENT_TASK_JOIN 176
#endif
#ifndef EVENT_USER_YIELD
    #define EVENT_USER_YIELD 194
    #define EVENT_SYS_YIELD  195
#endif

#endif
