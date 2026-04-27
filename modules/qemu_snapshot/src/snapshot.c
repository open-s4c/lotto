#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <qemu-plugin.h>

#include <lotto/modules/qemu/events.h>
#include <lotto/qemu/trap.h>

#define MAX_SNAPSHOTS 10

#define MAX_SNAPSHOT_STR_LEN 128

#define SNAPSHOT_PREFIX   "ls_"

typedef struct snapshot_state_s {
    uint32_t snapshot_nr;
    char snapshot_str[MAX_SNAPSHOT_STR_LEN];
} snapshot_state_t;

snapshot_state_t snapshot_state;

void qemu_plugin_delayed_save_snapshot(const char *name);

static void
qemu_snapshot_vcpu_resume_cb(qemu_plugin_id_t id, unsigned int vcpu_index)
{
    (void)id;
    printf("qemu_snapshot: vcpu %u resumed\n", vcpu_index);
}

void
qemu_snapshot(void)
{
    snapshot_state.snapshot_nr++;
    if (snapshot_state.snapshot_nr > MAX_SNAPSHOTS)
        snapshot_state.snapshot_nr = 1;

    snprintf(snapshot_state.snapshot_str, MAX_SNAPSHOT_STR_LEN,
             SNAPSHOT_PREFIX "%u", snapshot_state.snapshot_nr);

    qemu_plugin_delayed_save_snapshot(snapshot_state.snapshot_str);
}

static void
plugin_exit(void)
{
}

void
qemu_snapshot_trap(const struct lotto_trap_event *ev)
{
    (void)ev;
    qemu_snapshot();
}

void
qemu_snapshot_plugin_init(const qemu_control_event_t *ev)
{
    qemu_plugin_register_vcpu_resume_cb((qemu_plugin_id_t)ev->plugin_id,
                                        qemu_snapshot_vcpu_resume_cb);
}

void
qemu_snapshot_plugin_fini(const qemu_control_event_t *ev)
{
    (void)ev;
    plugin_exit();
}
