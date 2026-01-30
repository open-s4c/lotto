#include <stdbool.h>
#include <stdint.h>

#include <lotto/base/task_id.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/qlotto/gdb/gdb_connection.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/qlotto/gdb/handling/execute.h>
#include <lotto/qlotto/gdb/handling/query.h>
#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/sys/assert.h>
#include <lotto/util/casts.h>
// vsync
#include <vsync/atomic.h>

typedef struct _state_halter_s {
    vatomic32_t gdb_execution_state;
    task_id gdb_execution_tid;
    bool gdb_halt_handled;
    int32_t gdb_signal;
    stop_reason_t gdb_stop_reason;
    uint64_t gdb_stop_reason_n;
} _state_halter_t;

static _state_halter_t _state = {
    .gdb_execution_state = VATOMIC_INIT(GDB_EXECUTION_RUNNING),
    .gdb_execution_tid   = 1,
    .gdb_halt_handled    = true,
    .gdb_signal          = 0,
    .gdb_stop_reason     = STOP_REASON_NONE,
    .gdb_stop_reason_n   = 0,
};

bool qemu_gdb_get_wait(void);

void
gdb_srv_handle_halted()
{
    if (!gdb_is_halted_handled()) {
        if (gdb_srv_get_client_fd() != 0) {
            gdb_srv_send_T_info(gdb_srv_get_client_fd(), gdb_get_stop_signal());
        }
        gdb_halted_handled();
        if (gdb_srv_get_client_fd() == 0 && !qemu_gdb_get_wait()) {
            // if we do not have a client connected and we should not wait for a
            // client continue running
            gdb_execution_run();
        }
    }
    return;
}

void
gdb_set_stop_reason(stop_reason_t sr, uint64_t n)
{
    _state.gdb_stop_reason = sr;
}

stop_reason_t
gdb_get_stop_reason()
{
    return _state.gdb_stop_reason;
}

uint64_t
gdb_get_stop_reason_n()
{
    return _state.gdb_stop_reason_n;
}

const char *
gdb_get_stop_reason_str()
{
    return stop_reason_str(_state.gdb_stop_reason);
}

bool
gdb_has_stop_reason()
{
    return _state.gdb_stop_reason > STOP_REASON_NONE &&
           _state.gdb_stop_reason < STOP_REASON_END_;
}

int32_t
gdb_get_stop_signal()
{
    return _state.gdb_signal;
}

void
gdb_set_stop_signal(int32_t halt_signal)
{
    _state.gdb_signal = halt_signal;
}

bool
gdb_is_halted_handled()
{
    return _state.gdb_halt_handled;
}

void
gdb_halted_handled()
{
    _state.gdb_halt_handled = true;
    return;
}

int64_t
gdb_get_execution_halted_gdb_tid(void)
{
    return CAST_TYPE(int64_t, _state.gdb_execution_tid);
}

void
gdb_execution_halt(task_id tid)
{
    _state.gdb_execution_tid = tid;
    vatomic32_xchg(&_state.gdb_execution_state, GDB_EXECUTION_HALTED);
    _state.gdb_halt_handled = false;
    // rl_fprintf(stderr, "Halting on tid %lx\n", tid);
    uint32_t val =
        vatomic32_await_neq(&_state.gdb_execution_state, GDB_EXECUTION_HALTED);
    ASSERT(val == GDB_EXECUTION_RUNNING);
}

void
gdb_execution_halt_async(task_id tid)
{
    _state.gdb_execution_tid = tid;
    vatomic32_xchg(&_state.gdb_execution_state, GDB_EXECUTION_HALTING);
    // rl_fprintf(stderr, "Halting (async) on tid %lx\n", tid);
}

void
gdb_execution_run()
{
    _state.gdb_execution_tid = ANY_TASK;
    // lotto_execution_state = GDB_EXECUTION_RUNNING;
    vatomic32_xchg(&_state.gdb_execution_state, GDB_EXECUTION_RUNNING);
}

void
gdb_await_halted()
{
    vatomic32_await_neq(&_state.gdb_execution_state, GDB_EXECUTION_HALTED);
    return;
}

bool
gdb_execution_has_halted()
{
    return GDB_EXECUTION_HALTED == vatomic32_read(&_state.gdb_execution_state);
}

PS_SUBSCRIBE_INTERFACE(TOPIC_DEADLOCK_DETECTED,
                       { gdb_execution_halt(_state.gdb_execution_tid); })
