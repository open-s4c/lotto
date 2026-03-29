/**
 * @file events.h
 * @brief Autocept semantic ingress event identifiers.
 */
#ifndef LOTTO_MODULES_AUTOCEPT_EVENTS_H
#define LOTTO_MODULES_AUTOCEPT_EVENTS_H

#include <stdint.h>

#include <dice/types.h>
#include <lotto/base/value.h>

#define EVENT_AUTOCEPT_CALL 181

struct autocept_call_regs {
#if defined(__x86_64__)
    uintptr_t rdi;
    uintptr_t rsi;
    uintptr_t rdx;
    uintptr_t rcx;
    uintptr_t r8;
    uintptr_t r9;
    uintptr_t r10;
    uintptr_t r11;
#elif defined(__aarch64__)
    uintptr_t x0;
    uintptr_t x1;
    uintptr_t x2;
    uintptr_t x3;
    uintptr_t x4;
    uintptr_t x5;
    uintptr_t x6;
    uintptr_t x7;
#else
    #error "unsupported autocept architecture"
#endif
};

struct autocept_call_event {
    struct autocept_call_regs regs;
    const char *name;
    void (*func)(void);
    type_id src_type;
    struct value ret;
    uintptr_t retpc;
};

#endif
