/*
 */
#ifndef LOTTO_QEMU_INTERCEPTOR_H
#define LOTTO_QEMU_INTERCEPTOR_H

#include <lotto/qlotto/qemu/util.h>

void frontend_init(void);
void frontend_exit(void);
icounter_t *get_instruction_counter(void);
icounter_t *get_instruction_counter_wfe(void);
icounter_t *get_instruction_counter_wfi(void);

CPUARMState *qlotto_get_armcpu(unsigned int cpu_index);

#endif // LOTTO_QEMU_INTERCEPTOR_H
