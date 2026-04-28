#include <stdlib.h>

#include "state.h"
#include <lotto/base/flags.h>
#include <lotto/driver/exec.h>
#include <lotto/driver/flagmgr.h>
#include <lotto/driver/subcmd.h>

DECLARE_COMMAND_FLAG(QEMU_STDIN, "", "qemu-stdin", "",
                     "connect stdin to qemu child", flag_off())
FLAG_GETTER(qemu_stdin, QEMU_STDIN)

NEW_PUBLIC_COMMAND_FLAG(STRESS_QEMU, "Q", "qemu", "",
                        "run stress command through qemu wrapper", flag_off())
FLAG_GETTER(stress_qemu, STRESS_QEMU)

static subcmd_f *_trace_orig;

static bool
_qemu_stdin_devnull(const flags_t *flags)
{
    return flags_is_on(flags, flag_stress_qemu()) &&
           !flags_is_on(flags, flag_qemu_stdin());
}

static int
_qemu_trace_wrapper(args_t *args, flags_t *flags)
{
    ASSERT(_trace_orig != NULL);
    if (flags_is_on(flags, flag_stress_qemu())) {
        qemu_prefix_args(args, flags);
    }
    return _trace_orig(args, flags);
}

static void
_attach_qemu_flags(const char *name)
{
    subcmd_t *subcmd = subcmd_find(name);
    if (subcmd == NULL) {
        return;
    }
    flag_sel_add(subcmd->sel, flag_stress_qemu());
    flag_sel_add(subcmd->sel, flag_qemu_bin());
    flag_sel_add(subcmd->sel, flag_qemu_plugin_debug());
    flag_sel_add(subcmd->sel, flag_qemu_plugins());
    flag_sel_add(subcmd->sel, flag_qemu_mem());
    flag_sel_add(subcmd->sel, flag_qemu_cpu());
    flag_sel_add(subcmd->sel, flag_qemu_stdin());
    flag_sel_add(subcmd->sel, flag_qemu_no_common_args());
}

static void
_attach_qemu_trace_wrapper(void)
{
    subcmd_t *subcmd = subcmd_find("trace");
    if (subcmd == NULL) {
        return;
    }
    if (subcmd->func != _qemu_trace_wrapper) {
        if (_trace_orig == NULL) {
            _trace_orig = subcmd->func;
        }
        subcmd->func = _qemu_trace_wrapper;
    }
}

ON_DRIVER_REGISTER_COMMANDS({
    qemu_enable_stress_prefix(true);
    execute_set_stdin_devnull(_qemu_stdin_devnull);
    _attach_qemu_flags("stress");
    _attach_qemu_flags("run");
    _attach_qemu_flags("record");
    _attach_qemu_flags("trace");
    _attach_qemu_trace_wrapper();
})
