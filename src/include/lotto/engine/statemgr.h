#ifndef LOTTO_STATEMGR_H
#define LOTTO_STATEMGR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/base/record.h>
#include <lotto/base/slot.h>
#include <lotto/core/engine/events.h>
#include <lotto/engine/pubsub.h>
#include <lotto/sys/assert.h>
#include <vsync/common/macros.h>

typedef enum state_type {
    STATE_TYPE_EPHEMERAL  = 0, // Not recorded
    STATE_TYPE_START      = 1, // RECORD_START
    STATE_TYPE_CONFIG     = 2, // RECORD_CONFIG
    STATE_TYPE_PERSISTENT = 3, // RECORD_SCHED
    STATE_TYPE_FINAL      = 4, // RECORD_END
    STATE_TYPE_EMPTY      = 5,
    STATE_TYPE_END_       = 6,
} state_type_t;

/**
 * Registers a marshable object m for slot
 *
 * @param m marshable object
 * @param type the type of state
 *
 */
void statemgr_register(int slot, marshable_t *m, state_type_t type);

size_t statemgr_size(state_type_t type);

const void *statemgr_unmarshal(const void *buf, state_type_t type,
                               bool publish);
void *statemgr_marshal(void *buf, state_type_t type);
void statemgr_print(state_type_t type);
void statemgr_record_unmarshal(const record_t *r);

/**
 * Registers a printable state with statemgr.
 *
 * @param var the global variable
 * @param type either CONFIG, PERSISTENT, EPHEMERAL, or FINAL
 * @param print the body of a print function
 */
#define REGISTER_PRINTABLE_STATE(type, var, ...)                               \
    static void STATEMGR_PRINT(type)(const marshable_t *m)                     \
    {                                                                          \
        do {                                                                   \
            __VA_ARGS__;                                                       \
        } while (0);                                                           \
    }                                                                          \
    ON_REGISTRATION_PHASE({                                                   \
        log_printf("registering state mgr %s \n", #type);                      \
        (var).m =                                                              \
            MARSHABLE_STATIC_PRINTABLE(sizeof(var), STATEMGR_PRINT(type));     \
        statemgr_register(DICE_MODULE_SLOT, &(var).m, STATE_TYPE_##type);      \
    })

#define REGISTER_CONFIG(var, ...)                                              \
    static void STATEMGR_PRINT(CONFIG)(const marshable_t *m)                   \
    {                                                                          \
        do {                                                                   \
            __VA_ARGS__;                                                       \
        } while (0);                                                           \
    }                                                                          \
    ON_REGISTRATION_PHASE({                                                   \
        (var).m =                                                              \
            MARSHABLE_STATIC_PRINTABLE(sizeof(var), STATEMGR_PRINT(CONFIG));   \
        statemgr_register(DICE_MODULE_SLOT, &(var).m, STATE_TYPE_CONFIG);      \
    })


#define REGISTER_CONFIG_NONSTATIC(var, mi)                                     \
    ON_REGISTRATION_PHASE({                                                   \
        (var).m = mi;                                                          \
        statemgr_register(DICE_MODULE_SLOT, &(var).m, STATE_TYPE_CONFIG);      \
    })

/**
 * Registers a static state with statemgr.
 *
 * @param var the global variable
 * @param type either CONFIG, PERSISTENT, EPHEMERAL, or FINAL
 * @param callback the body of an unmarshal callback function (can be empty)
 */
#define REGISTER_STATIC_STATE(type, var, ...)                                  \
    static void STATEMGR_CALLBACK(type)(void)                                  \
    {                                                                          \
        do {                                                                   \
            __VA_ARGS__;                                                       \
        } while (0);                                                           \
    }                                                                          \
    ON_REGISTRATION_PHASE({                                                   \
        (var).m = MARSHABLE_STATIC(sizeof(var));                               \
        STATEMGR_CALLBACK(type)();                                             \
        statemgr_register(DICE_MODULE_SLOT, &(var).m, STATE_TYPE_##type);      \
    })

/**
 * Registers a dynamic state with statemgr.
 *
 * @param var the global variable
 * @param type either CONFIG, PERSISTENT, EPHEMERAL, or FINAL
 * @param init the body of an initialization function
 */
#define REGISTER_STATE(type, var, ...)                                         \
    static void STATEMGR_INIT(type)(marshable_t * m)                           \
    {                                                                          \
        do {                                                                   \
            __VA_ARGS__;                                                       \
        } while (0);                                                           \
    }                                                                          \
    ON_REGISTRATION_PHASE({                                                   \
        STATEMGR_INIT(type)((marshable_t *)&var);                              \
        statemgr_register(DICE_MODULE_SLOT, (marshable_t *)&var,               \
                          STATE_TYPE_##type);                                  \
    })

#define REGISTER_PERSISTENT(var, ...)                                          \
    REGISTER_STATE(PERSISTENT, var, __VA_ARGS__)

#define REGISTER_EPHEMERAL(var, ...) REGISTER_STATE(EPHEMERAL, var, __VA_ARGS__)

#define STATEMGR_REGISTER(type, ...)                                           \
    ON_REGISTRATION_PHASE({                                                   \
        do {                                                                   \
            __VA_ARGS__                                                        \
        } while (0);                                                           \
    })

#define STATEMGR_PRINT(type)    _statemgr_##type##_print
#define STATEMGR_INIT(type)     _statemgr_##type##_init
#define STATEMGR_CALLBACK(type) _statemgr_##type##_callback

#endif
