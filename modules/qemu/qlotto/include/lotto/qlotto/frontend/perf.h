/*
 */
#ifndef LOTTO_FRONTEND_PERF_H
#define LOTTO_FRONTEND_PERF_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/task_id.h>
#include <lotto/qlotto/qemu/callbacks.h>

void vcpu_trace_start_capture(unsigned int cpu_index, void *udata);
void vcpu_trace_end_capture(unsigned int cpu_index, void *udata);

void frontend_perf_init(void);
void frontend_perf_exit(void);

#endif // LOTTO_FRONTEND_PERF_H
