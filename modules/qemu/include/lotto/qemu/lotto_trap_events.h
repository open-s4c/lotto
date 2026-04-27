/**
 * @file lotto_trap_events.h
 * @brief Semantic event ids exposed to the QEMU guest trap ABI.
 */
#ifndef LOTTO_QEMU_LOTTO_TRAP_EVENTS_H
#define LOTTO_QEMU_LOTTO_TRAP_EVENTS_H

#include <lotto/modules/bias/events.h>
#include <lotto/modules/deadlock/events.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu_snapshot/events.h>
#include <lotto/modules/terminate/events.h>
#include <lotto/modules/yield/events.h>

/*
 * Keep this local until mutex exposes a lightweight event-id-only header that
 * does not pull Dice/runtime payload types into bare-metal guest builds.
 */
#define EVENT_MUTEX_TRYACQUIRE 143
#define EVENT_WORKGROUP_JOIN   192

#endif
