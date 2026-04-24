#include <qemu-plugin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dice/chains/intercept.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/modules/qemu/events.h>
#include <lotto/modules/qemu_snapshot/events.h>
#include <lotto/modules/qemu_snapshot/final.h>
#include <lotto/qemu/trap.h>
#include <lotto/sys/string.h>
#include <lotto/terminate.h>

#define MAX_SNAPSHOTS 10

#define MAX_SNAPSHOT_STR_LEN 128

#define SNAPSHOT_PREFIX "ls_"

typedef struct snapshot_state_s {
    uint32_t snapshot_nr;
    bool pending;
    bool completed_valid;
    bool completed_success;
    char completed_name[MAX_SNAPSHOT_STR_LEN];
    char snapshot_str[MAX_SNAPSHOT_STR_LEN];
} snapshot_state_t;

snapshot_state_t snapshot_state;

static void
qemu_snapshot_done_cb(qemu_plugin_id_t id, bool success)
{
    (void)id;

    if (!snapshot_state.pending) {
        return;
    }

    struct qemu_snapshot_done_event ev = {
        .name = snapshot_state.snapshot_str,
        .success = success,
    };

    snapshot_state.completed_valid = true;
    snapshot_state.completed_success = success;
    sys_strcpy(snapshot_state.completed_name, snapshot_state.snapshot_str);
    qemu_snapshot_final_note(snapshot_state.snapshot_str, success);
    snapshot_state.pending = false;
    PS_PUBLISH(INTERCEPT_EVENT, EVENT_QEMU_SNAPSHOT_DONE, &ev, NULL);
    lotto_terminate(TERMINATE_ABANDON);
}

void
qemu_snapshot(void)
{
    snapshot_state.snapshot_nr++;
    if (snapshot_state.snapshot_nr > MAX_SNAPSHOTS)
        snapshot_state.snapshot_nr = 1;

    snprintf(snapshot_state.snapshot_str, MAX_SNAPSHOT_STR_LEN,
             SNAPSHOT_PREFIX "%u", snapshot_state.snapshot_nr);

    if (!qemu_plugin_delayed_save_snapshot(snapshot_state.snapshot_str)) {
        printf(
            "qemu_snapshot: snapshot request ignored while another save is "
            "pending\n");
        return;
    }

    snapshot_state.pending = true;
}

static void
plugin_exit(void)
{
}

void
qemu_snapshot_trap(const capture_point *trap_cp, metadata_t *md)
{
    (void)trap_cp;
    (void)md;
    qemu_snapshot();
}

void
qemu_snapshot_plugin_init(const qemu_control_event_t *ev)
{
    qemu_plugin_register_snapshot_done_cb((qemu_plugin_id_t)ev->plugin_id,
                                          qemu_snapshot_done_cb);
}

void
qemu_snapshot_plugin_fini(const qemu_control_event_t *ev)
{
    (void)ev;
    plugin_exit();
}

LOTTO_SUBSCRIBE_SEQUENCER_CAPTURE(EVENT_QEMU_SNAPSHOT_DONE, {
    const capture_point *cp = EVENT_PAYLOAD(cp);
    sequencer_decision *e = cp->decision;
    e->should_record = true;
    e->readonly = true;
    e->next = cp->id;
    qemu_snapshot_final_set_clk(e->clk);
})
