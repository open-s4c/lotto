#ifndef LOTTO_CORE_MODULE_H
#define LOTTO_CORE_MODULE_H

#include <dice/module.h>

#define LOTTO_MODULE_INIT(...)                                                 \
    DICE_MODULE_INIT()                                                         \
    static void __attribute__((constructor)) init_()                           \
    {                                                                          \
        do {                                                                   \
            __VA_ARGS__                                                        \
        } while (0);                                                           \
    }
//    PS_SUBSCRIBE(CHAIN_CONTROL, EVENT_DICE_READY, {

#endif /* LOTTO_CORE_MODULE_H */
