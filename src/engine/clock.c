/*
 */

/*
 * TODO:
 * - implement TOPIC for capture and clock increment
 * - implement FLAGS for start_time (base == start_time?)
 * - migrate code from engine
 * - consolidate _seq.clk with _clock.ticks
 * - define clock sources/types ? (capture, wall-clock..)
 * - define user-fine-tune of clock (base and drift is sufficient?)
 * - refactor whole thing with new clock definition
 * - callibrate/autotune
 */

#include <limits.h>
#include <stdint.h>
#include <stdio.h>

#define LOGGER_PREFIX LOGGER_CUR_FILE

#include <lotto/base/clk.h>
#include <lotto/base/context.h>
#include <lotto/base/marshable.h>
#include <lotto/base/topic.h>
#include <lotto/base/value.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/engine/clock.h>
#include <lotto/sys/logger.h>
#include <lotto/util/casts.h>
#include <lotto/util/contract.h>

struct config {
    marshable_t m;
    uint64_t start_time;
};

struct clock {
    marshable_t m;
    clk_t clk;
    uint64_t icount;
    uint64_t icount_offset;
    uint64_t icount_no_offset_count;
    uint64_t icount_no_offset_count_max;
};

static struct config _config;
static struct clock _clock;


static void _clock_tick(uint64_t icount);

PS_SUBSCRIBE_INTERFACE(TOPIC_AFTER_UNMARSHAL_CONFIG, {
    marshable_print_m(&_config);
    _clock.icount_no_offset_count_max = 10;
})

PS_SUBSCRIBE_INTERFACE(TOPIC_BEFORE_CAPTURE, {
#if defined(QLOTTO_ENABLED)
    const context_t *ctx = (const context_t *)as_any(v);
    _clock_tick(ctx->icount);
#else
    _clock_tick(0);
#endif
})

static void
_clock_tick(uint64_t icount)
{
    _clock.clk++;

    ASSERT(icount == 0 || icount >= _clock.icount);

    if (icount > 0) {
        _clock.icount                 = icount;
        _clock.icount_no_offset_count = 0;
    } else {
        _clock.icount_no_offset_count++;
        if (_clock.icount_no_offset_count < _clock.icount_no_offset_count_max)
            _clock.icount_offset += ICOUNT_OFFSET_INC;
        else
            _clock.icount_offset += (long)ICOUNT_OFFSET_INC * 30;
    }
}


static uint64_t
_clock_icount()
{
    return _clock.icount + _clock.icount_offset;
}

uint64_t
clock_ns()
{
    uint64_t icount     = _clock_icount();
    uint64_t base_ns    = icount * NS_PER_INST;
    double intermediate = (double)base_ns / CLOCK_DRIFT;
    uint64_t cur_ns     = CAST_TYPE(uint64_t, (int64_t)intermediate);
    return cur_ns;
}

void
clock_time(struct timespec *ts)
{
    uint64_t cur_ns = clock_ns();

    uint64_t sec  = cur_ns / NSEC_IN_SEC;
    uint64_t nsec = cur_ns % NSEC_IN_SEC;

    ASSERT(sec < LONG_MAX);
    ts->tv_sec = (long)sec;
    ASSERT(nsec < LONG_MAX);
    ts->tv_nsec = (long)nsec;
}

void
clock_print(void)
{
    struct timespec ts;
    clock_time(&ts);
    logger_debugf("Lotto clock: %5lu sec %09lu nsec\n", ts.tv_sec, ts.tv_nsec);
}

void
lotto_clock_set(const struct timespec *ts)
{
    struct timespec now;
    clock_time(&now);
    int cmp = timespec_compare(ts, &now);
    if (cmp == 0) {
        return;
    }
    ASSERT(timespec_compare(ts, &now) > 0);
    uint64_t diff_nsec =
        (ts->tv_sec - now.tv_sec) * NSEC_IN_SEC + ts->tv_nsec - now.tv_nsec;
    uint64_t diff_icount = CAST_TYPE(
        uint64_t, (int64_t)(((double)(diff_nsec + NS_PER_INST) / NS_PER_INST) *
                            CLOCK_DRIFT));
    _clock.icount_offset += diff_icount;
    CONTRACT({
        clock_time(&now);
        ASSERT(timespec_compare(ts, &now) <= 0);
    })
}

int
timespec_compare(const struct timespec *ts1, const struct timespec *ts2)
{
    return ts1->tv_sec < ts2->tv_sec   ? -1 :
           ts1->tv_sec > ts2->tv_sec   ? 1 :
           ts1->tv_nsec < ts2->tv_nsec ? -1 :
           ts1->tv_nsec > ts2->tv_nsec ? 1 :
                                         0;
}
