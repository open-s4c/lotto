#include <inttypes.h>
#include <stdint.h>

#include "profile_emit.h"
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>

static struct {
    int enabled;
    uint64_t tb_count;
    uint64_t insn_count;
    uint64_t mem_reads;
    uint64_t mem_writes;
    uint64_t udf_exit_count;
    uint64_t udf_yield_count;
    uint64_t wfe_count;
    uint64_t wfi_count;
    int summary_printed;
} g_qemu_profile;

static inline int
profile_is_enabled(void)
{
    return __atomic_load_n(&g_qemu_profile.enabled, __ATOMIC_RELAXED) == 1;
}

static int
env_enabled(const char *name)
{
    const char *value = sys_getenv(name);
    return value != NULL && value[0] == '1';
}

void
qemu_profile_on_init(void)
{
    sys_memset(&g_qemu_profile, 0, sizeof(g_qemu_profile));
    g_qemu_profile.enabled = env_enabled("LOTTO_QEMU_PROFILE") ? 1 : 0;
}

void
qemu_profile_on_fini(void)
{
    if (!profile_is_enabled()) {
        return;
    }
    if (__atomic_exchange_n(&g_qemu_profile.summary_printed, 1,
                            __ATOMIC_RELAXED) != 0) {
        return;
    }

    sys_fprintf(
        stderr,
        "qemu-profile: tb=%" PRIu64 " insn=%" PRIu64 " mem-r=%" PRIu64
        " mem-w=%" PRIu64 " udf-exit=%" PRIu64 " udf-yield=%" PRIu64
        " wfe=%" PRIu64 " wfi=%" PRIu64 "\n",
        __atomic_load_n(&g_qemu_profile.tb_count, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.insn_count, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.mem_reads, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.mem_writes, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.udf_exit_count, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.udf_yield_count, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.wfe_count, __ATOMIC_RELAXED),
        __atomic_load_n(&g_qemu_profile.wfi_count, __ATOMIC_RELAXED));
}

void
qemu_profile_on_translate(size_t insn_count)
{
    if (!profile_is_enabled()) {
        return;
    }
    __atomic_fetch_add(&g_qemu_profile.tb_count, 1u, __ATOMIC_RELAXED);
    __atomic_fetch_add(&g_qemu_profile.insn_count, (uint64_t)insn_count,
                       __ATOMIC_RELAXED);
}

void
qemu_profile_on_memaccess(bool is_store)
{
    if (!profile_is_enabled()) {
        return;
    }

    if (is_store) {
        __atomic_fetch_add(&g_qemu_profile.mem_writes, 1u, __ATOMIC_RELAXED);
        return;
    }

    __atomic_fetch_add(&g_qemu_profile.mem_reads, 1u, __ATOMIC_RELAXED);
}

void
qemu_profile_on_udf_yield(void)
{
    if (!profile_is_enabled()) {
        return;
    }
    __atomic_fetch_add(&g_qemu_profile.udf_yield_count, 1u, __ATOMIC_RELAXED);
}
