/*
 */

// system
#include <stdbool.h>

// capstone
#include <capstone/arm64.h>
#include <capstone/capstone.h>

// lotto
#include <lotto/check.h>
#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/fd_stub.h>
#include <lotto/qlotto/gdb/gdb_connection.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/gdb_server.h>
#include <lotto/qlotto/gdb/gdb_server_stub.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/interceptor.h>
#include <lotto/qlotto/mapping.h>
#include <lotto/qlotto/qemu/callbacks.h>
#include <lotto/sys/ensure.h>

// qemu
#include <qemu-plugin.h>

struct plugin_gdb {
    bool gdb_server_started;
} _state_plugin_gdb;

/*******************************************************************************
 * plugin
 ******************************************************************************/

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

uint64_t inst_loop_count = 0;
csh disasm;

static void
vcpu_tb_trans(qemu_plugin_id_t id, struct qemu_plugin_tb *tb)
{
    size_t n = qemu_plugin_tb_n_insns(tb);
    cs_insn *insn_cs;
    uint32_t opcode;

    if (!_state_plugin_gdb.gdb_server_started) {
        _state_plugin_gdb.gdb_server_started = true;
        gdb_server_start();
    }

    for (size_t i = 0; i < n; i++) {
        struct qemu_plugin_insn *insn = qemu_plugin_tb_get_insn(tb, i);
        void *pc                      = (void *)qemu_plugin_insn_vaddr(insn);

        qemu_plugin_register_vcpu_insn_exec_cb(
            insn, gdb_capture, QEMU_PLUGIN_CB_NO_REGS, (void *)pc);

        opcode = get_instruction_data(insn);
        if (cs_disasm(disasm, (const unsigned char *)&opcode, sizeof(opcode),
                      (uintptr_t)pc, 0, &insn_cs) == 1) {
            int insn_cat = mapping_arm64[insn_cs->id];
            switch (insn_cat) {
                case CAT_BEFORE_READ:
                case CAT_BEFORE_WRITE:
                case CAT_BEFORE_AREAD:
                case CAT_BEFORE_AWRITE:
                case CAT_BEFORE_CMPXCHG:
                case CAT_BEFORE_RMW:
                    qemu_plugin_register_vcpu_mem_cb(
                        insn, gdb_handle_mem_capture, QEMU_PLUGIN_CB_R_REGS,
                        QEMU_PLUGIN_MEM_RW, NULL);
                default:
                    break;
            }
        }
    }
}

static void
plugin_exit(qemu_plugin_id_t id, void *p)
{
    // lotto exit is done by plugin-lotto
}

bool qemu_gdb_enabled(void);

QEMU_PLUGIN_EXPORT int
qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                    char **argv)
{
    ENSURE(lotto_loaded());

    fd_stub_init();

    arm_cpu_init();
    gdb_send_init();
    gdb_srv_connection_init();
    gdb_srv_stub_init();
    gdb_break_init();

    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);
    REGISTER_CALL(gdb_register_cpu)
    ENSURE(cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &disasm) == CS_ERR_OK);
    ENSURE(cs_option(disasm, CS_OPT_DETAIL, CS_OPT_ON) == CS_ERR_OK);

    _state_plugin_gdb.gdb_server_started = false;
    return 0;
}
