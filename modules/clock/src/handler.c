#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "state.h"
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/statemgr.h>
#include <lotto/modules/clock.h>
#include <lotto/modules/clock/events.h>
#include <lotto/runtime/capture_point.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/util/casts.h>
#include <lotto/util/contract.h>

typedef struct clock_state {
    clk_t clk;
    uint64_t cost;
    uint64_t cost_offset;
    uint64_t no_cost_count;
} clock_state_t;

static clock_state_t _clock;

static void _clock_tick(uint64_t cpu_cost);
static uint64_t _clock_cost(void);
static uint64_t _clock_ns(void);
static void _clock_time_local(struct timespec *ts);

REGISTER_EPHEMERAL(_clock, ({ _clock = (clock_state_t){0}; }))

static void
_clock_tick(uint64_t cpu_cost)
{
    clock_config_t *cfg = clock_config();

    _clock.clk++;

    ASSERT(cpu_cost == 0 || cpu_cost >= _clock.cost);

    if (cpu_cost > 0) {
        _clock.cost          = cpu_cost;
        _clock.no_cost_count = 0;
    } else {
        _clock.no_cost_count++;
        if (_clock.no_cost_count < cfg->max_gap)
            _clock.cost_offset += cfg->base_inc * cfg->mult_inc;
        else
            _clock.cost_offset += cfg->base_inc * cfg->burst_gap;
    }
}

static uint64_t
_clock_cost(void)
{
    return _clock.cost + _clock.cost_offset;
}

static uint64_t
_clock_ns(void)
{
    uint64_t cost       = _clock_cost();
    uint64_t base_ns    = cost * NS_PER_INST;
    double intermediate = (double)base_ns / CLOCK_DRIFT;
    return CAST_TYPE(uint64_t, (int64_t)intermediate);
}

void
_clock_time_local(struct timespec *ts)
{
    uint64_t cur_ns = _clock_ns();

    uint64_t sec  = cur_ns / NSEC_IN_SEC;
    uint64_t nsec = cur_ns % NSEC_IN_SEC;

    ASSERT(sec < LONG_MAX);
    ts->tv_sec = (long)sec;
    ASSERT(nsec < LONG_MAX);
    ts->tv_nsec = (long)nsec;
}

void
lotto_clock_time(struct timespec *ts)
{
    _clock_time_local(ts);
}

void
lotto_clock_print(void)
{
    struct timespec ts;
    _clock_time_local(&ts);
    logger_debugf("Lotto clock: %5lu sec %09lu nsec\n", ts.tv_sec, ts.tv_nsec);
}

void
lotto_clock_leap(const struct timespec *ts)
{
    struct timespec now;
    _clock_time_local(&now);
    int cmp = timespec_compare(ts, &now);
    if (cmp == 0) {
        return;
    }
    ASSERT(timespec_compare(ts, &now) > 0);
    int64_t diff_nsec = (int64_t)(ts->tv_sec - now.tv_sec) * NSEC_IN_SEC +
                        (int64_t)(ts->tv_nsec - now.tv_nsec);
    ASSERT(diff_nsec > 0);
    double diff_cost_d = ceil(((double)diff_nsec / NS_PER_INST) * CLOCK_DRIFT);
    uint64_t diff_cost = CAST_TYPE(uint64_t, (int64_t)diff_cost_d);
    _clock.cost_offset += diff_cost;
    _clock_time_local(&now);
    while (timespec_compare(ts, &now) > 0) {
        _clock.cost_offset++;
        _clock_time_local(&now);
    }
    CONTRACT({ ASSERT(timespec_compare(ts, &now) <= 0); })
}

ON_SEQUENCER_RESUME({
    if (cp->type_id == EVENT_CLOCK_READ)
        e->skip = true;
})

ON_SEQUENCER_CAPTURE({
    if (cp->type_id == EVENT_CLOCK_READ) {
        struct lotto_clock_event *ev = cp->payload;
        ev->ret                      = _clock_ns();
        e->next                      = cp->id;
        e->readonly                  = true;
        e->skip                      = true;
    } else {
        _clock_tick(cp->cpu_cost);
    }
})
