#include <dice/module.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/qemu_snapshot/final.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/string.h>

typedef struct qemu_snapshot_pending_final_state {
    bool valid;
    bool success;
    clk_t clk;
    char name[QEMU_SNAPSHOT_NAME_MAX];
} qemu_snapshot_pending_final_state_t;

typedef struct qemu_snapshot_final_wire_state {
    bool valid;
    bool success;
    clk_t clk;
    char name[QEMU_SNAPSHOT_NAME_MAX];
} qemu_snapshot_final_wire_state_t;

static qemu_snapshot_pending_final_state_t _pending;

static void
_print_final(const marshable_t *m)
{
    const struct qemu_snapshot_final_state *s =
        (const struct qemu_snapshot_final_state *)m;

    logger_infof("snapshot valid=%s success=%s clk=%lu name=%s\n",
                 s->valid ? "true" : "false", s->success ? "true" : "false",
                 s->clk, s->name[0] ? s->name : "<none>");
}

static struct qemu_snapshot_final_state _final;

static size_t _final_size(const marshable_t *m);
static void *_final_marshal(const marshable_t *m, void *buf);
static const void *_final_unmarshal(marshable_t *m, const void *buf);

static const marshable_t _final_marshable = {
    .marshal    = _final_marshal,
    .unmarshal  = _final_unmarshal,
    .size       = _final_size,
    .print      = _print_final,
    .alloc_size = sizeof(struct qemu_snapshot_final_state),
    .next       = NULL,
};

static void DICE_CTOR
qemu_snapshot_state_init_(void)
{
    _final = (struct qemu_snapshot_final_state){
        .m = _final_marshable,
    };
    statemgr_register(DICE_MODULE_SLOT, (marshable_t *)&_final,
                      STATE_TYPE_FINAL);
}

LOTTO_SUBSCRIBE(EVENT_ENGINE__BEFORE_MARSHAL_FINAL, {
    (void)v;

    _final.valid   = _pending.valid;
    _final.success = _pending.success;
    _final.clk     = _pending.clk;
    if (_pending.valid) {
        sys_strcpy(_final.name, _pending.name);
    } else {
        _final.name[0] = '\0';
    }
})

struct qemu_snapshot_final_state *
qemu_snapshot_final_state(void)
{
    return &_final;
}

void
qemu_snapshot_final_note(const char *name, bool success)
{
    _pending.valid   = true;
    _pending.success = success;
    if (name != NULL) {
        sys_strcpy(_pending.name, name);
    } else {
        _pending.name[0] = '\0';
    }

    _final.valid   = _pending.valid;
    _final.success = _pending.success;
    sys_strcpy(_final.name, _pending.name);
}

void
qemu_snapshot_final_set_clk(clk_t clk)
{
    _pending.clk = clk;
    _final.clk   = clk;
}

static size_t
_final_size(const marshable_t *m)
{
    (void)m;
    return sizeof(qemu_snapshot_final_wire_state_t);
}

static void *
_final_marshal(const marshable_t *m, void *buf)
{
    const struct qemu_snapshot_final_state *s =
        (const struct qemu_snapshot_final_state *)m;
    qemu_snapshot_final_wire_state_t wire = {
        .valid   = s->valid,
        .success = s->success,
        .clk     = s->clk,
    };

    sys_strcpy(wire.name, s->name);
    sys_memcpy(buf, &wire, sizeof(wire));
    return (char *)buf + sizeof(wire);
}

static const void *
_final_unmarshal(marshable_t *m, const void *buf)
{
    struct qemu_snapshot_final_state *s = (struct qemu_snapshot_final_state *)m;
    qemu_snapshot_final_wire_state_t wire;

    sys_memcpy(&wire, buf, sizeof(wire));
    s->valid   = wire.valid;
    s->success = wire.success;
    s->clk     = wire.clk;
    sys_strcpy(s->name, wire.name);

    return (const char *)buf + sizeof(wire);
}
