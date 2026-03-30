#include <lotto/base/marshable.h>
#include <lotto/base/stable_address.h>
#include <lotto/base/string.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/state.h>
#include <lotto/modules/enforce/state.h>
#include <lotto/runtime/events.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/time.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

#define PASTE(a, b) a##b
#define EQUAL_CP(x) (enforce_state()->cp.PASTE(, x) == cp->PASTE(, x))
#define EQUAL_PC                                                               \
    (pc = stable_address_get(cp->pc,                                           \
                             sequencer_config()->stable_address_method),       \
     stable_address_equals(&enforce_state()->pc, &pc))
#define EQUAL_SEED (enforce_state()->seed == prng_seed())
#define MODE(x)    (enforce_modes_has(enforce_config()->modes, ENFORCE_MODE_##x))
#define EQUAL_ADDR (enforce_state()->addr == memaccess_addr(cp))

static bool
_uses_current_mem_value(const capture_point *cp)
{
    switch (cp->src_type) {
        case EVENT_MA_READ:
        case EVENT_MA_WRITE:
            return cp->src_chain == CHAIN_INGRESS_EVENT;
        case EVENT_MA_AREAD:
        case EVENT_MA_AWRITE:
            return cp->src_chain == CHAIN_INGRESS_BEFORE;
        default:
            return false;
    }
}

static arg_t
_read_val(const arg_t *ptr, size_t width)
{
    uintptr_t p = ptr->value.ptr;
    ASSERT(ptr->width == ARG_PTR);
    arg_t v = {.width = (enum arg_width)width, .value = {0}};
    switch (width) {
        case 8:
            v.value.u64 = *(uint64_t *)p;
            break;
        case 4:
            v.value.u32 = *(uint32_t *)p;
            break;
        case 2:
            v.value.u16 = *(uint16_t *)p;
            break;
        case 1:
            v.value.u8 = *(uint8_t *)p;
            break;
        default:
            logger_fatalf("unexpected width %u\n", ptr->width);
    }
    return v;
}

bool
_as_expected(const capture_point *cp)
{
    stable_address_t pc;
    bool has_addr = has_memaccess_addr(cp);
    if ((MODE(TID) && !EQUAL_CP(id)) ||
        (MODE(CAT) && enforce_state()->cp.src_type != cp->src_type) ||
        (MODE(PC) && !EQUAL_PC) || (MODE(ADDRESS) && has_addr && !EQUAL_ADDR) ||
        (MODE(SEED) && !EQUAL_SEED))
        return false;

    if (MODE(DATA) && _uses_current_mem_value(cp)) {
        arg_t p = arg_ptr((void *)memaccess_addr(cp));
        arg_t a = _read_val(&p, memaccess_size(cp));
        if (enforce_state()->val.value.u64 != a.value.u64) {
            return false;
        }
    }

    return true;
}

#define REPORT_CTX(fmt, F, x)                                                  \
    REPORT(fmt, F, x, enforce_state()->cp.PASTE(, x), cp->PASTE(, x))

#define REPORT(fmt, F, n, x, y)                                                \
    do {                                                                       \
        logger_errorf("MISMATCH [field: %s, expected: " fmt ", actual: " fmt   \
                      "]\n",                                                   \
                      #n, F(x), F(y));                                         \
    } while (0)

#define _(X) X

static void
_report(const capture_point *cp)
{
    bool has_addr = has_memaccess_addr(cp);
    stable_address_t pc;
    if (!EQUAL_CP(id))
        REPORT_CTX("%lu", _, id);
    if (MODE(CAT) && enforce_state()->cp.src_type != cp->src_type)
        REPORT("%s", ps_type_str, cat, enforce_state()->cp.src_type,
               cp->src_type);
    if (MODE(ADDRESS) && has_addr && !EQUAL_ADDR)
        REPORT("%lx", _, addr, enforce_state()->addr, memaccess_addr(cp));
    if (MODE(DATA) && _uses_current_mem_value(cp)) {
        arg_t p = arg_ptr((void *)memaccess_addr(cp));
        arg_t a = _read_val(&p, memaccess_size(cp));
        if (enforce_state()->val.value.u64 != a.value.u64) {
            logger_errorf("MISMATCH [field: val, expected: %lu, actual: %lu]\n",
                          enforce_state()->val.value.u64, a.value.u64);
        }
    }
    if (MODE(PC) && !EQUAL_PC)
        REPORT_CTX("%p", (void *), pc);

    if (MODE(SEED) && !EQUAL_SEED) {
        REPORT("%lu", _, seed, enforce_state()->seed, prng_seed());
    }
}


void
_save(const capture_point *cp, const event_t *e)
{
    switch (cp->src_type) {
        case EVENT_MA_READ:
        case EVENT_MA_AREAD:
        case EVENT_MA_WRITE:
        case EVENT_MA_AWRITE:
            if (MODE(DATA)) {
                arg_t p              = arg_ptr((void *)memaccess_addr(cp));
                enforce_state()->val = _read_val(&p, memaccess_size(cp));
            }
            break;

        default:
            break;
    }
    enforce_state()->clk = e->clk;
    if (MODE(CAT) || MODE(TID) || MODE(ADDRESS)) {
        enforce_state()->cp = *cp;
        if (MODE(ADDRESS) && has_memaccess_addr(cp)) {
            enforce_state()->addr = memaccess_addr(cp);
        }
    }
    if (MODE(PC)) {
        enforce_state()->pc = stable_address_get(
            cp->pc, sequencer_config()->stable_address_method);
    }
    if (MODE(SEED)) {
        enforce_state()->seed = prng_seed();
    }
}

static void
check_aslr()
{
    char *randomize_va_space = "/proc/sys/kernel/randomize_va_space";
    FILE *fp                 = sys_fopen(randomize_va_space, "r");
    if (!fp) {
        logger_warnf("Can't read ASLR status\n");
    } else {
        char aslr[1];
        sys_fread(aslr, 1, 1, fp);
        if (aslr[0] != '0' &&
            (MODE(PC) || MODE(ADDRESS) || MODE(DATA) || MODE(CUSTOM)) &&
            sequencer_config()->stable_address_method ==
                STABLE_ADDRESS_METHOD_NONE) {
            logger_warnf("ASLR enabled but no stable address method\n");
        }
    }
}

void
_handle(const capture_point *ctx, event_t *cp)
{
    once(check_aslr());
    if (enforce_config()->modes == ENFORCE_MODE_NONE)
        return;
    if (cp->replay && cp->clk == enforce_state()->clk) {
        if (!_as_expected(ctx)) {
            logger_errorf(
                "Replay mismatch! cappt = [clk: %lu, id: %lu, type: %s, pc: "
                "%p]\n",
                cp->clk, ctx->id, ps_type_str(ctx->src_type), (void *)ctx->pc);
            _report(ctx);
            logger_fatalf("unexpected capture point\n");
            sys_abort();
        }
    }
    _save(ctx, cp);
}
ON_SEQUENCER_CAPTURE(_handle)
