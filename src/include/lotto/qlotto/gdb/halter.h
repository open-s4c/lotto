#ifndef LOTTO_GDB_HALTER_H
#define LOTTO_GDB_HALTER_H

#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/task_id.h>
#include <lotto/qlotto/gdb/handling/stop_reason.h>

typedef enum execution_state_s {
    GDB_EXECUTION_RUNNING,
    GDB_EXECUTION_HALTING,
    GDB_EXECUTION_HALTED,
} execution_state_t;

void gdb_execution_halt(task_id tid);
void gdb_execution_halt_async(task_id tid);
void gdb_execution_run(void);
void gdb_await_halted(void);
bool gdb_execution_has_halted(void);
void gdb_set_stop_signal(int32_t halt_signal);
int32_t gdb_get_stop_signal(void);
bool gdb_is_halted_handled(void);
void gdb_halted_handled(void);
void gdb_srv_handle_halted();
int64_t gdb_get_execution_halted_gdb_tid(void);
void gdb_set_stop_reason(stop_reason_t sr, uint64_t n);
stop_reason_t gdb_get_stop_reason(void);
uint64_t gdb_get_stop_reason_n(void);
const char *gdb_get_stop_reason_str(void);
bool gdb_has_stop_reason();

#endif // LOTTO_GDB_HALTER_H
