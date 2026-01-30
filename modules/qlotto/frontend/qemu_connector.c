/*
 */

#include <signal.h>

// lotto
#include <lotto/base/context.h>
#include <lotto/base/reason.h>
#include <lotto/check.h>
#include <lotto/qlotto/frontend/intercept_translation.h>
#include <lotto/qlotto/qemu/callbacks.h>
#include <lotto/runtime/intercept.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/string.h>

// qemu
#include <qemu-plugin.h>

#if defined(LOTTO_PERF_EXAMPLE)
void qemu_cpu_kick_cb(void *cpu);
void register_qemu_cpu_kick_cb(void (*func)(void *cpu));
#endif

/*******************************************************************************
 * plugin
 ******************************************************************************/

static void
plugin_exit(qemu_plugin_id_t id, void *p)
{
    instrumentation_exit();
    lotto_exit(ctx(.func = __FUNCTION__), REASON_SUCCESS);
}

static void
_handle_sigill(int sig, siginfo_t *si, void *arg)
{
    // UDF executed
    // ignore signal
}

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

QEMU_PLUGIN_EXPORT int
qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                    char **argv)
{
    ENSURE(lotto_loaded());
    REGISTER_CALL(qlotto_register_cpu)
    instrumentation_init();

    qemu_plugin_register_vcpu_tb_trans_cb(id, vcpu_tb_trans);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);

    {
        struct sigaction action;
        sys_memset(&action, 0, sizeof(struct sigaction));
        action.sa_flags     = SA_SIGINFO;
        action.sa_sigaction = _handle_sigill;
        sigaction(SIGILL, &action, NULL);
    }

    return 0;
}
