#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <dice/chains/capture.h>
#include <dice/module.h>
#include <dice/pubsub.h>
#include <dice/self.h>
#include <lotto/modules/autocept/events.h>

static volatile double _fp_sink;
static volatile uint64_t _int_sink;

struct checker_state {
    bool seen_before;
    struct autocept_call_event event;
};
static struct checker_state _checker_state_key;

__attribute__((noinline)) static void
_checker_clobber_fp(void)
{
#if defined(__x86_64__)
    __asm__ volatile(
        "pxor %%xmm0, %%xmm0\n\t"
        "pxor %%xmm1, %%xmm1\n\t"
        "pxor %%xmm2, %%xmm2\n\t"
        "pxor %%xmm3, %%xmm3\n\t"
        "pxor %%xmm4, %%xmm4\n\t"
        "pxor %%xmm5, %%xmm5\n\t"
        "pxor %%xmm6, %%xmm6\n\t"
        "pxor %%xmm7, %%xmm7\n\t"
        :
        :
        : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7");
#endif
}

__attribute__((noinline)) static double
_checker_mix_fp(double a, double b, double c, double d)
{
    return ((a + b) * 1.25) - ((c - d) * 0.5);
}

__attribute__((noinline)) static uint64_t
_checker_mix_int(uint64_t a, uint64_t b, uint64_t c, uint64_t d, uint64_t e,
                 uint64_t f)
{
    return (a ^ (b << 7)) + (c >> 3) + (d * 17) - (e ^ f);
}

static void
_checker_stress(const struct autocept_call_event *ev)
{
    assert(ev != NULL);
    assert(ev->name != NULL);

    _checker_clobber_fp();
    _fp_sink += _checker_mix_fp(1.25, 2.5, 3.75, 4.0);
    _int_sink ^= _checker_mix_int((uintptr_t)ev->name, (uintptr_t)ev->func,
                                  UINT64_C(0x13579bdf), UINT64_C(0x2468ace0),
                                  UINT64_C(0xfeedface), UINT64_C(0xabcdef01));
}

static void
_checker_report(const char *stage)
{
    printf("  %s OK\n", stage);
    fflush(stdout);
}

PS_SUBSCRIBE(CAPTURE_BEFORE, EVENT_AUTOCEPT_CALL, {
    struct checker_state *state          = SELF_TLS(md, &_checker_state_key);
    const struct autocept_call_event *ev = EVENT_PAYLOAD(event);

    assert(md != NULL);
    assert(md->drop == false);
    md->drop           = true;
    state->seen_before = true;
    state->event       = *ev;
    _checker_stress(ev);
    _checker_report("before");
    return PS_OK;
})

PS_SUBSCRIBE(CAPTURE_AFTER, EVENT_AUTOCEPT_CALL, {
    struct checker_state *state          = SELF_TLS(md, &_checker_state_key);
    const struct autocept_call_event *ev = EVENT_PAYLOAD(event);

    assert(md != NULL);
    assert(state->seen_before);
    assert(memcmp(&state->event.regs, &ev->regs, sizeof(ev->regs)) == 0);
    assert(state->event.name == ev->name);
    assert(state->event.retpc == ev->retpc);
    assert(md->drop == true);
    _checker_stress(ev);
    state->seen_before = false;
    _checker_report("after ");
    return PS_OK;
})
