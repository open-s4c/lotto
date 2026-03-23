#define LOGGER_BLOCK LOGGER_CUR_BLOCK
#include <lotto/base/marshable.h>
#include <lotto/base/stable_address.h>
#include <lotto/base/string.h>
#include <dice/pubsub.h>
#include <lotto/engine/prng.h>
#include <lotto/engine/pubsub.h>
#include <lotto/engine/sequencer.h>
#include <lotto/engine/state.h>
#include <lotto/modules/enforce/state.h>
#include <dice/chains/capture.h>
#include <lotto/runtime/memaccess_payload.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/sys/time.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

#define PASTE(a, b) a##b
#define EQUAL_CP(x) (enforce_state()->cp.PASTE(, x) == cp->PASTE(, x))
#define EQUAL_PC                                                               \
    (pc = stable_address_get(cp->pc,                                           \
                             sequencer_config()->stable_address_method),       \
     stable_address_equals(&enforce_state()->pc, &pc))
#define EQUAL_DATA                                                             \
    (sys_memcmp(enforce_state()->data, (char *)context_memaccess_addr(cp),     \
                context_memaccess_size(cp)) == 0 &&                            \
     (context_memaccess_size(cp) == ENFORCE_DATA_SIZE ||                       \
      (((char *)context_memaccess_addr(cp))[context_memaccess_size(cp)] ==     \
           0 &&                                                                \
       sys_memcmp((char *)context_memaccess_addr(cp) +                         \
                      context_memaccess_size(cp),                              \
                  (char *)context_memaccess_addr(cp) +                         \
                      context_memaccess_size(cp) + 1,                          \
                  ENFORCE_DATA_SIZE - context_memaccess_size(cp) - 1) == 0)))
#define EQUAL_SEED (enforce_state()->seed == prng_seed())
#define MODE(x)    (enforce_modes_has(enforce_config()->modes, ENFORCE_MODE_##x))
#define EQUAL_ADDR (enforce_state()->addr == context_memaccess_addr(cp))

static type_id
_effective_type(const capture_point *cp)
{
    return cp->src_type;
}

static context_memaccess_event_t
_memaccess_event(const capture_point *cp)
{
    switch (cp->src_type) {
        case EVENT_MA_READ:
            return CONTEXT_MA_BEFORE_READ;
        case EVENT_MA_WRITE:
            return CONTEXT_MA_BEFORE_WRITE;
        case EVENT_MA_AREAD:
            return cp->src_chain == CAPTURE_AFTER ? CONTEXT_MA_AFTER_AREAD :
                                                   CONTEXT_MA_BEFORE_AREAD;
        case EVENT_MA_AWRITE:
            return cp->src_chain == CAPTURE_AFTER ? CONTEXT_MA_AFTER_AWRITE :
                                                   CONTEXT_MA_BEFORE_AWRITE;
        case EVENT_MA_RMW:
            return cp->src_chain == CAPTURE_AFTER ? CONTEXT_MA_AFTER_RMW :
                                                   CONTEXT_MA_BEFORE_RMW;
        case EVENT_MA_XCHG:
            return cp->src_chain == CAPTURE_AFTER ? CONTEXT_MA_AFTER_XCHG :
                                                   CONTEXT_MA_BEFORE_XCHG;
        case EVENT_MA_CMPXCHG:
        case EVENT_MA_CMPXCHG_WEAK:
            return cp->src_chain == CAPTURE_AFTER ?
                       CONTEXT_MA_AFTER_CMPXCHG_S :
                       CONTEXT_MA_BEFORE_CMPXCHG;
        case EVENT_MA_FENCE:
            return cp->src_chain == CAPTURE_AFTER ? CONTEXT_MA_AFTER_FENCE :
                                                   CONTEXT_MA_BEFORE_FENCE;
        default:
            return CONTEXT_MA_NONE;
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
    context_memaccess_event_t ma = _memaccess_event(cp);
    bool has_memaccess           = ma != CONTEXT_MA_NONE;
    if ((MODE(TID) && !EQUAL_CP(id)) ||
        (MODE(CAT) && enforce_state()->cp.src_type != _effective_type(cp)) ||
        (MODE(PC) && !EQUAL_PC) ||
        (MODE(ADDRESS) && has_memaccess && !EQUAL_ADDR) ||
        (MODE(SEED) && !EQUAL_SEED))
        return false;

    switch (ma) {
        case CONTEXT_MA_BEFORE_READ:
        case CONTEXT_MA_BEFORE_AREAD:
        case CONTEXT_MA_BEFORE_WRITE:
        case CONTEXT_MA_BEFORE_AWRITE:
            if (MODE(DATA)) {
                arg_t p = arg_ptr((void *)context_memaccess_addr(cp));
                arg_t a = _read_val(&p, context_memaccess_size(cp));
                if (enforce_state()->val.value.u64 != a.value.u64) {
                    return false;
                }
            }
            break;

        default:
            break;
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
    context_memaccess_event_t ma = _memaccess_event(cp);
    bool has_memaccess           = ma != CONTEXT_MA_NONE;
    stable_address_t pc;
    if (!EQUAL_CP(id))
        REPORT_CTX("%lu", _, id);
    if (MODE(CAT) && enforce_state()->cp.src_type != _effective_type(cp))
        REPORT("%s", ps_type_str, cat, enforce_state()->cp.src_type,
               _effective_type(cp));
    if (MODE(ADDRESS) && has_memaccess && !EQUAL_ADDR)
        REPORT("%lx", _, addr, enforce_state()->addr,
               context_memaccess_addr(cp));
    if (MODE(DATA) &&
        (ma == CONTEXT_MA_BEFORE_READ || ma == CONTEXT_MA_BEFORE_AREAD)) {
        arg_t p = arg_ptr((void *)context_memaccess_addr(cp));
        arg_t a = _read_val(&p, context_memaccess_size(cp));
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

LOTTO_ADVERTISE_TYPE(EVENT_ENFORCE__VIOLATED)

void
_save(const capture_point *cp, const event_t *e)
{
    context_memaccess_event_t ma = _memaccess_event(cp);
    bool has_memaccess           = ma != CONTEXT_MA_NONE;

    switch (_effective_type(cp)) {
        case EVENT_MA_READ:
        case EVENT_MA_AREAD:
        case EVENT_MA_WRITE:
        case EVENT_MA_AWRITE:
            if (MODE(DATA)) {
                arg_t p = arg_ptr((void *)context_memaccess_addr(cp));
                enforce_state()->val =
                    _read_val(&p, context_memaccess_size(cp));
            }
            break;

        default:
            break;
    }
    enforce_state()->clk = e->clk;
    if (MODE(CAT) || MODE(TID) || MODE(ADDRESS)) {
        enforce_state()->cp = *cp;
        enforce_state()->cp.src_type = _effective_type(cp);
        if (MODE(ADDRESS) && has_memaccess) {
            enforce_state()->addr = context_memaccess_addr(cp);
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
                cp->clk, ctx->id, ps_type_str(_effective_type(ctx)),
                (void *)ctx->pc);
            _report(ctx);
            logger_fatalf("unexpected capture point\n");
            sys_abort();
        }
    }
    _save(ctx, cp);
}
REGISTER_SEQUENCER_HANDLER(_handle)
