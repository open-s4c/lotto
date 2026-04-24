
// system
#include <stdbool.h>

// lotto
#include "gdb/arm_cpu.h"
#include "gdb/fd_stub.h"
#include "gdb/gdb_connection.h"
#include "gdb/gdb_send.h"
#include "gdb/gdb_server.h"
#include "gdb/gdb_server_stub.h"
#include "gdb/halter.h"
#include "gdb/handling/break.h"
#include "gdb/interceptor.h"
#include <lotto/check.h>
#include <lotto/modules/qemu/callbacks.h>
#include <lotto/modules/qemu_gdb/qemu_gdb.h>
#include <lotto/sys/ensure.h>

// qemu
#include <qemu-plugin.h>

#include <lotto/modules/qemu/events.h>

struct plugin_gdb {
    bool gdb_server_started;
} _state_plugin_gdb;

static void
vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);

    if (!_state_plugin_gdb.gdb_server_started) {
        _state_plugin_gdb.gdb_server_started = true;
        gdb_server_start();
    }

    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        void *pc                      = (void *)qemu_plugin_insn_vaddr(insn);

        qemu_plugin_register_vcpu_insn_exec_cb(
            insn, gdb_capture, QEMU_PLUGIN_CB_NO_REGS, (void *)pc);
        qemu_plugin_register_vcpu_mem_cb(insn, gdb_handle_mem_capture,
                                         QEMU_PLUGIN_CB_R_REGS,
                                         QEMU_PLUGIN_MEM_RW, NULL);
    }
}

static void
plugin_exit(qemu_plugin_id_t id, void *p)
{
    (void)id;
    (void)p;
    // lotto exit is done by plugin-lotto
}

void
qemu_gdb_translate(const qemu_translate_event_t *ev)
{
    if (ev == NULL) {
        return;
    }
    vcpu_tb_trans((qemu_plugin_id_t)ev->plugin_id, ev->tb);
}

void
qemu_gdb_plugin_init(const qemu_control_event_t *ev)
{
    (void)ev;

    if (!qemu_gdb_enabled()) {
        return;
    }

    ENSURE(lotto_loaded());

    fd_stub_init();

    arm_cpu_init();
    gdb_send_init();
    gdb_srv_connection_init();
    gdb_srv_stub_init();
    gdb_break_init();

    REGISTER_CALL(gdb_register_cpu)

    _state_plugin_gdb.gdb_server_started = false;
}

void
qemu_gdb_plugin_fini(const qemu_control_event_t *ev)
{
    if (!qemu_gdb_enabled()) {
        return;
    }
    plugin_exit((qemu_plugin_id_t)ev->plugin_id, NULL);
}
