#include <assert.h>
#include <string.h>

#include "../../../modules/pos/src/handler.c"
#include <lotto/base/trace_flat.h>
#include <lotto/engine/recorder.h>

static pos_config_t config    = {.enabled = true};
static sequencer_config_t seq = {.strategy = "pos"};
static uint64_t seed;

pos_config_t *
pos_config(void)
{
    return &config;
}
sequencer_config_t *
sequencer_config(void)
{
    return &seq;
}
uint64_t
prng_next(void)
{
    return seed++;
}

void
statemgr_register(int slot, marshable_t *m, state_type_t type)
{
}
size_t
statemgr_size(state_type_t type)
{
    return 0;
}
void *
statemgr_marshal(void *buf, state_type_t type)
{
    return buf;
}
void
statemgr_record_unmarshal(const record_t *r)
{
    assert(r->kind == RECORD_CONFIG);
    memcpy(&seed, r->data, sizeof(seed));
}

static void
append_config(trace_t *trace, clk_t clk, uint64_t value, bool reseed)
{
    record_t *r = record_alloc(sizeof(value));
    r->kind     = RECORD_CONFIG;
    r->clk      = clk;
    r->type_id  = reseed ? EVENT_ENGINE__RESEED : 0;
    memcpy(r->data, &value, sizeof(value));
    assert(trace_append(trace, r) == TRACE_OK);
}

int
main(void)
{
    tidmap_init(&_state, MARSHABLE_TASK);
    task_t *a   = (task_t *)tidmap_register(&_state, 1);
    task_t *b   = (task_t *)tidmap_register(&_state, 2);
    a->priority = 900;
    b->priority = 800;
    a->addr = b->addr = 42;
    a->is_write       = true;
    b->is_write       = false;

    trace_t *input  = trace_flat_create(NULL);
    trace_t *output = trace_flat_create(NULL);
    append_config(input, 0, 10, false);
    append_config(input, 1, 50, true);
    LOTTO_PUBLISH(EVENT_ENGINE__START, nil);
    recorder_init(input, output);
    recorder_replay(1);
    assert(seed == 10 && a->priority == 900 && b->priority == 800);
    recorder_replay(2);
    assert(seed == 52);
    assert(a->priority != b->priority);
    assert(a->priority >= 50 && a->priority < 52);
    assert(b->priority >= 50 && b->priority < 52);
    assert(a->addr == 42 && b->addr == 42 && a->is_write && !b->is_write);
    assert(trace_last(output)->type_id == EVENT_ENGINE__RESEED);
    uint64_t ap = a->priority, bp = b->priority;

    a->priority = 100;
    b->priority = 200;
    LOTTO_PUBLISH(EVENT_ENGINE__START, nil);
    recorder_init(output, NULL);
    recorder_replay(1);
    assert(a->priority == 100 && b->priority == 200);
    recorder_replay(2);
    assert(a->priority == ap && b->priority == bp && seed == 52);

    config.enabled = false;
    LOTTO_PUBLISH(EVENT_ENGINE__RESEED, nil);
    assert(seed == 52 && a->priority == ap && b->priority == bp);
    config.enabled = true;
    strcpy(seq.strategy, "random");
    LOTTO_PUBLISH(EVENT_ENGINE__RESEED, nil);
    assert(seed == 52 && a->priority == ap && b->priority == bp);
    return 0;
}
