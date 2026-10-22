/**
 * @file events.h
 * @brief Driver declarations for events.
 */
#ifndef LOTTO_CORE_DRIVER_EVENTS_H
#define LOTTO_CORE_DRIVER_EVENTS_H

/** Driver event: initialize memory-manager-related state. */
#ifndef DICE_ROLLED
    #define EVENT_DRIVER__MEMMGR_INIT 133
#endif
/** Driver event: register CLI flags contributed by modules. */
#ifndef DICE_ROLLED
    #define EVENT_DRIVER__REGISTER_FLAGS 134
#endif
/** Driver event: register CLI subcommands contributed by modules. */
#ifndef DICE_ROLLED
    #define EVENT_DRIVER__REGISTER_COMMANDS 135
#endif

#endif
