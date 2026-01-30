/*
 * QEMU ARM CPU -- internal functions and types
 *
 * Copyright (c) 2014 Linaro Ltd
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
 *
 * This header defines functions, types, etc which need to be shared
 * between different source files within target/arm/ but which are
 * private to it and not required by the rest of QEMU.
 */

#ifndef TARGET_ARM_INTERNALS_H
#define TARGET_ARM_INTERNALS_H

typedef uint64_t target_ulong;

/**
 * ARMFaultType: type of an ARM MMU fault
 * This corresponds to the v8A pseudocode's Fault enumeration,
 * with extensions for QEMU internal conditions.
 */
typedef enum ARMFaultType {
    ARMFault_None,
    ARMFault_AccessFlag,
    ARMFault_Alignment,
    ARMFault_Background,
    ARMFault_Domain,
    ARMFault_Permission,
    ARMFault_Translation,
    ARMFault_AddressSize,
    ARMFault_SyncExternal,
    ARMFault_SyncExternalOnWalk,
    ARMFault_SyncParity,
    ARMFault_SyncParityOnWalk,
    ARMFault_AsyncParity,
    ARMFault_AsyncExternal,
    ARMFault_Debug,
    ARMFault_TLBConflict,
    ARMFault_UnsuppAtomicUpdate,
    ARMFault_Lockdown,
    ARMFault_Exclusive,
    ARMFault_ICacheMaint,
    ARMFault_QEMU_NSCExec, /* v8M: NS executing in S&NSC memory */
    ARMFault_QEMU_SFault,  /* v8M: SecureFault INVTRAN, INVEP or AUVIOL */
} ARMFaultType;

/**
 * ARMMMUFaultInfo: Information describing an ARM MMU Fault
 * @type: Type of fault
 * @level: Table walk level (for translation, access flag and permission faults)
 * @domain: Domain of the fault address (for non-LPAE CPUs only)
 * @s2addr: Address that caused a fault at stage 2
 * @stage2: True if we faulted at stage 2
 * @s1ptw: True if we faulted at stage 2 while doing a stage 1 page-table walk
 * @s1ns: True if we faulted on a non-secure IPA while in secure state
 * @ea: True if we should set the EA (external abort type) bit in syndrome
 */
typedef struct ARMMMUFaultInfo ARMMMUFaultInfo;
struct ARMMMUFaultInfo {
    ARMFaultType type;
    target_ulong s2addr;
    int level;
    int domain;
    bool stage2;
    bool s1ptw;
    bool s1ns;
    bool ea;
};

#endif