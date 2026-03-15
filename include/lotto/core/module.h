#ifndef LOTTO_CORE_MODULE_H
#define LOTTO_CORE_MODULE_H

#include <dice/module.h>

/*
 * Define the Lotto startup hook for the current process/module unit.
 *
 * This is the point where a runtime or driver enters Lotto startup and begins
 * publishing its explicit phases.
 *
 * The actual phase API lives in `lotto/engine/pubsub.h`, typically:
 *   - START_REGISTRATION_PHASE()
 *   - START_INITIALIZATION_PHASE()
 *
 * This macro is still constructor-backed, so it should stay limited to process
 * startup wiring rather than ordinary module behavior.
 */
#define LOTTO_MODULE_INIT(...)                                                 \
    DICE_MODULE_INIT()                                                         \
    static void __attribute__((constructor)) init_()                           \
    {                                                                          \
        do {                                                                   \
            __VA_ARGS__                                                        \
        } while (0);                                                           \
    }

#endif /* LOTTO_CORE_MODULE_H */
