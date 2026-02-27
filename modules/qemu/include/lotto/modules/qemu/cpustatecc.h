/*
 * QEMU CPU model
 *
 * Copyright (c) 2012 SUSE LINUX Products GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see
 * <http://www.gnu.org/licenses/gpl-2.0.html>
 */

#ifndef QEMU_CPU_H
#define QEMU_CPU_H

#include <stdint.h>

// We skip the first entry, so it is not the same as CPUState in QEMU!
// Get CPUStateCC* by &(CPUState*)->cc
// Get CPUStateCC* by (CPUState*) + offsetof(CPUState, cc)

typedef struct CPUStateCC {
    /*< private >*/
    // DeviceState parent_obj;
    /* cache to avoid expensive CPU_GET_CLASS */
    // CPUClass *cc;
    void *cc;
    /*< public >*/

    int nr_cores;
    int nr_threads;

    void *thread;
#ifdef _WIN32
    QemuSemaphore sem;
#endif
    bool stopped;

    /* Should CPU start in powered-off state? */
    bool start_powered_off;

    bool unplug;
    bool crash_occurred;
    bool exit_request;
    int exclusive_context_count;
    uint32_t cflags_next_tb;
    /* updates protected by BQL */
    uint32_t interrupt_request;
    int singlestep_enabled;
    int64_t icount_budget;
    int64_t icount_extra;
    uint64_t random_seed;

    // a lot comes after this, but we shorten the state as it is enough and
    // easier to handle
} CPUStateCC;

#endif // QEMU_CPU_H
