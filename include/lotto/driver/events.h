/**
 * @file events.h
 * @brief Driver declarations for events.
 */
#ifndef LOTTO_CORE_DRIVER_EVENTS_H
#define LOTTO_CORE_DRIVER_EVENTS_H

/** Driver event: initialize memory-manager-related state. */
#define EVENT_DRIVER__MEMMGR_INIT 133
/** Driver event: register CLI flags contributed by modules. */
#define EVENT_DRIVER__REGISTER_FLAGS 134
/** Driver event: register CLI subcommands contributed by modules. */
#define EVENT_DRIVER__REGISTER_COMMANDS 135

#endif
