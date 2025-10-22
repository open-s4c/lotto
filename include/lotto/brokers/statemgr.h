#ifndef LOTTO_STATEMGR_H
#define LOTTO_STATEMGR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lotto/base/marshable.h>
#include <lotto/base/record.h>
#include <vsync/common/macros.h>

typedef enum state_type {
    STATE_TYPE_EPHEMERAL,  // Not recorded
    STATE_TYPE_START,      // RECORD_START
    STATE_TYPE_CONFIG,     // RECORD_CONFIG
    STATE_TYPE_PERSISTENT, // RECORD_SCHED
    STATE_TYPE_FINAL,      // RECORD_END
    STATE_TYPE_EMPTY,
    STATE_TYPE_END_,
} state_type_t;

/**
 * Registers a marshable object m for slot
 *
 * @param m marshable object
 * @param type the type of state
 *
 */
void statemgr_register(marshable_t *m, state_type_t type);

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
#define REGISTER_PRINTABLE_STATE(type, var, print)                             \
    static void STATEMGR_PRINT(type)(const marshable_t *m)                     \
    {                                                                          \
        do {                                                                   \
            (print);                                                           \
        } while (0);                                                           \
    }                                                                          \
    static void LOTTO_CONSTRUCTOR STATEMGR_CONSTRUCTOR(type)(void)             \
    {                                                                          \
        (var).m =                                                              \
            MARSHABLE_STATIC_PRINTABLE(sizeof(var), STATEMGR_PRINT(type));     \
        statemgr_register(&(var).m, STATE_TYPE_##type);                        \
    }


#define REGISTER_CONFIG(var, print)                                            \
    static void STATEMGR_PRINT(CONFIG)(const marshable_t *m)                   \
    {                                                                          \
        do {                                                                   \
            (print);                                                           \
        } while (0);                                                           \
    }                                                                          \
    static void LOTTO_CONSTRUCTOR STATEMGR_CONSTRUCTOR(CONFIG)(void)           \
    {                                                                          \
        (var).m =                                                              \
            MARSHABLE_STATIC_PRINTABLE(sizeof(var), STATEMGR_PRINT(CONFIG));   \
        statemgr_register(&(var).m, STATE_TYPE_CONFIG);                        \
    }

#define REGISTER_CONFIG_NONSTATIC(var, mi)                                     \
    static void LOTTO_CONSTRUCTOR STATEMGR_CONSTRUCTOR(CONFIG)(void)           \
    {                                                                          \
        (var).m = mi;                                                          \
        statemgr_register(&(var).m, STATE_TYPE_CONFIG);                        \
    }

/**
 * Registers a static state with statemgr.
 *
 * @param var the global variable
 * @param type either CONFIG, PERSISTENT, EPHEMERAL, or FINAL
 * @param callback the body of an unmarshal callback function (can be empty)
 */
#define REGISTER_STATIC_STATE(type, var, callback)                             \
    static void STATEMGR_CALLBACK(type)(void)                                  \
    {                                                                          \
        do {                                                                   \
            (callback);                                                        \
        } while (0);                                                           \
    }                                                                          \
    static void LOTTO_CONSTRUCTOR STATEMGR_CONSTRUCTOR(type)(void)             \
    {                                                                          \
        (var).m = MARSHABLE_STATIC(sizeof(var));                               \
        statemgr_register(&(var).m, STATEMGR_CALLBACK(type),                   \
                          STATE_TYPE_##type);                                  \
    }


/**
 * Registers a dynamic state with statemgr.
 *
 * @param var the global variable
 * @param type either CONFIG, PERSISTENT, EPHEMERAL, or FINAL
 * @param init the body of an initialization function
 */
#define REGISTER_STATE(type, var, init)                                        \
    static void STATEMGR_INIT(type)(marshable_t * m)                           \
    {                                                                          \
        do {                                                                   \
            (init);                                                            \
        } while (0);                                                           \
    }                                                                          \
    static void LOTTO_CONSTRUCTOR STATEMGR_CONSTRUCTOR(type)(void)             \
    {                                                                          \
        STATEMGR_INIT(type)((marshable_t *)&var);                              \
        statemgr_register((marshable_t *)&var, STATE_TYPE_##type);             \
    }


#define REGISTER_PERSISTENT(var, init) REGISTER_STATE(PERSISTENT, var, init)

#define REGISTER_EPHEMERAL(var, init) REGISTER_STATE(EPHEMERAL, var, init)

#define STATEMGR_PRINT(type)       _statemgr_##type##_print
#define STATEMGR_INIT(type)        _statemgr_##type##_init
#define STATEMGR_CONSTRUCTOR(type) _statemgr_##type##_constructor

#endif
