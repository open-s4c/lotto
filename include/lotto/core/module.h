#ifndef LOTTO_CORE_MODULE_H
#define LOTTO_CORE_MODULE_H

#include <dice/module.h>

#define LOTTO_MODULE_INIT(...)                                                 \
    DICE_MODULE_INIT()                                                         \
    LOTTO_MODULE_INIT_({                                                       \
        do {                                                                   \
            __VA_ARGS__                                                        \
        } while (0);                                                           \
    })

#if 0
    #define LOTTO_MODULE_INIT_(...)                                            \
        PS_SUBSCRIBE(CHAIN_CONTROL, EVENT_DICE_READY, __VA_ARGS__)
#else
    #define LOTTO_MODULE_INIT_(...)                                            \
        static void __attribute__((constructor(1002)))                         \
        V_JOIN(lotto_init, __COUNTER__)(void)                                  \
        {                                                                      \
            __VA_ARGS__                                                        \
        }
#endif
#endif /* LOTTO_CORE_MODULE_H */
