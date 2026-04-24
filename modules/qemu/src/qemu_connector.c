#include <qemu-plugin.h>
#include <signal.h>

#include "interceptors.h"
#include "translate_aarch64.h"
#include <lotto/base/reason.h>
#include <lotto/check.h>
#include <lotto/modules/qemu/callbacks.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/runtime/runtime.h>
#include <lotto/sys/ensure.h>
#include <lotto/sys/string.h>

static void
plugin_exit(qemu_plugin_id_t id, void *p)
{
    (void)id;
    (void)p;
    qemu_on_plugin_stop();
    qemu_emit_fini();
    capture_point cp = {.func = __FUNCTION__};
    lotto_exit(&cp, REASON_SUCCESS);
}

static void
handle_sigill(int sig, siginfo_t *si, void *arg)
{
    (void)sig;
    (void)si;
    (void)arg;
}

QEMU_PLUGIN_EXPORT int qemu_plugin_version = QEMU_PLUGIN_VERSION;

QEMU_PLUGIN_EXPORT int
qemu_plugin_install(qemu_plugin_id_t id, const qemu_info_t *info, int argc,
                    char **argv)
{
    ENSURE(lotto_loaded());
    register_qlotto_register_cpu(qemu_register_cpu_context);
    qemu_emit_init(id, info, argc, argv);
    qemu_on_plugin_start(id, info, argc, argv);
    qemu_plugin_register_vcpu_tb_trans_cb(id, qemu_dispatch_tb_translate);
    qemu_plugin_register_atexit_cb(id, plugin_exit, NULL);

    {
        struct sigaction action;
        sys_memset(&action, 0, sizeof(struct sigaction));
        action.sa_flags     = SA_SIGINFO;
        action.sa_sigaction = handle_sigill;
        sigaction(SIGILL, &action, NULL);
    }

    return 0;
}
