#ifndef LOTTO_CORE_MODULE_H
#define LOTTO_CORE_MODULE_H

#include <dice/module.h>

#define LOTTO_MODULE_INIT_JOIN_(a, b) a##b
#define LOTTO_MODULE_INIT_JOIN(a, b)  LOTTO_MODULE_INIT_JOIN_(a, b)

#define LOTTO_MODULE_INIT(...)                                                \
    DICE_MODULE_INIT()                                                        \
    static void __attribute__((constructor(10000)))                           \
        LOTTO_MODULE_INIT_JOIN(lotto_module_init_, __LINE__)(void)            \
    {                                                                         \
        do {                                                                  \
            __VA_ARGS__                                                       \
        } while (0);                                                          \
    }

#endif /* LOTTO_CORE_MODULE_H */
