/**
 * @file interceptor.h
 * @brief QLotto frontend declarations for interceptor.
 */
#ifndef LOTTO_QEMU_INTERCEPTOR_H
#define LOTTO_QEMU_INTERCEPTOR_H

#include <stdbool.h>

#include <lotto/base/cappt.h>
#include <lotto/base/context.h>
#include <lotto/modules/qemu/util.h>

void frontend_init(void);
void frontend_exit(void);
icounter_t *get_instruction_counter(void);
icounter_t *get_instruction_counter_wfe(void);
icounter_t *get_instruction_counter_wfi(void);

CPUARMState *qlotto_get_armcpu(unsigned int cpu_index);
bool qlotto_frontend_ready(void);
void qlotto_set_frontend_ready(bool ready);
void qlotto_capture_handle(const context_t *ctx, event_t *e);

#endif // LOTTO_QEMU_INTERCEPTOR_H
