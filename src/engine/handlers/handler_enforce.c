/*
 */
#define LOG_PREFIX LOG_CUR_FILE
#define LOG_BLOCK  LOG_CUR_BLOCK
#include <lotto/base/marshable.h>
#include <lotto/base/stable_address.h>
#include <lotto/base/string.h>
#include <lotto/brokers/catmgr.h>
#include <lotto/brokers/pubsub.h>
#include <lotto/engine/dispatcher.h>
#include <lotto/engine/prng.h>
#include <lotto/states/handlers/enforce.h>
#include <lotto/states/sequencer.h>
#include <lotto/sys/assert.h>
#include <lotto/sys/logger_block.h>
#include <lotto/sys/stdio.h>
#include <lotto/sys/stdlib.h>
#include <lotto/sys/string.h>
#include <lotto/sys/time.h>
#include <lotto/util/macros.h>
#include <lotto/util/once.h>

#define PASTE(a, b) a##b
#define EQUAL(x)    (enforce_state()->ctx.PASTE(, x) == ctx->PASTE(, x))
#define EQUAL_PC                                                               \
    (pc = stable_address_get(ctx->pc,                                          \
                             sequencer_config()->stable_address_method),       \
     stable_address_equals(&enforce_state()->pc, &pc))
#define EQUAL_DATA                                                             \
    (sys_memcmp(enforce_state()->data, (char *)ctx->args[0].value.ptr,         \
                ctx->args[1].value.u64) == 0 &&                                \
     (ctx->args[1].value.u64 == ENFORCE_DATA_SIZE ||                           \
      (((char *)ctx->args[0].value.ptr)[ctx->args[1].value.u64] == 0 &&        \
       sys_memcmp((char *)ctx->args[0].value.ptr + ctx->args[1].value.u64,     \
                  (char *)ctx->args[0].value.ptr + ctx->args[1].value.u64 + 1, \
                  ENFORCE_DATA_SIZE - ctx->args[1].value.u64 - 1) == 0)))
#define EQUAL_SEED (enforce_state()->seed == prng_seed())
#define MODE(x)    (enforce_modes_has(enforce_config()->modes, ENFORCE_MODE_##x))

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
            log_fatalf("unexpected width %u\n", ptr->width);
    }
    return v;
}

bool
_as_expected(const context_t *ctx)
{
    stable_address_t pc;
    if ((MODE(TID) && !EQUAL(id)) || (MODE(CAT) && !EQUAL(cat)) ||
        (MODE(PC) && !EQUAL_PC) ||
        (MODE(ADDRESS) && !EQUAL(args[0].value.u64)) ||
        (MODE(SEED) && !EQUAL_SEED))
        return false;

    switch (ctx->cat) {
        case CAT_BEFORE_READ:
        case CAT_BEFORE_AREAD:
        case CAT_BEFORE_WRITE:
        case CAT_BEFORE_AWRITE:
            if (MODE(DATA)) {
                arg_t a = _read_val(&ctx->args[0], ctx->args[1].value.u64);
                if (enforce_state()->val.value.u64 != a.value.u64) {
                    return false;
                }
            }
            break;

        case CAT_ENFORCE:
            if (!MODE(CUSTOM)) {
                break;
            }
            ASSERT(ctx->args[0].width == ARG_PTR);
            ASSERT(ctx->args[1].value.u64 <= ENFORCE_DATA_SIZE);
            if (!EQUAL_DATA) {
                return false;
            }
            break;

        default:
            break;
    }

    return true;
}

#define REPORT_CTX(fmt, F, x)                                                  \
    REPORT(fmt, F, x, enforce_state()->ctx.PASTE(, x), ctx->PASTE(, x))

#define REPORT(fmt, F, n, x, y)                                                \
    do {                                                                       \
        log_errorf("MISMATCH [field: %s, expected: " fmt ", actual: " fmt      \
                   "]\n",                                                      \
                   #n, F(x), F(y));                                            \
    } while (0)

#define _(X) X

static void
_report(const context_t *ctx)
{
    stable_address_t pc;
    if (!EQUAL(id))
        REPORT_CTX("%lu", _, id);
    if (!EQUAL(cat))
        REPORT_CTX("%s", category_str, cat);
    if (MODE(ADDRESS) && !EQUAL(args[0].value.u64))
        REPORT_CTX("%lx", _, args[0].value.u64);
    if (MODE(DATA) &&
        (ctx->cat == CAT_BEFORE_READ || ctx->cat == CAT_BEFORE_AREAD)) {
        arg_t a = _read_val(&ctx->args[0], ctx->args[1].value.u64);
        if (enforce_state()->val.value.u64 != a.value.u64) {
            log_errorf("MISMATCH [field: val, expected: %lu, actual: %lu]\n",
                       enforce_state()->val.value.u64, a.value.u64);
        }
    }
    if (!EQUAL_PC)
        REPORT_CTX("%p", (void *), pc);

    if (ctx->cat == CAT_ENFORCE && !EQUAL_DATA) {
        struct value val = on();
        PS_PUBLISH_INTERFACE(TOPIC_ENFORCE_VIOLATED, val);
        log_errorf("MISMATCH [field: enforce, expected: ");
        for (size_t i = 0; i < ENFORCE_DATA_SIZE; i++) {
            log_errorf("%2.2x", enforce_state()->data[i]);
        }
        log_errorf(", actual: ");
        for (size_t i = 0; i < ctx->args[1].value.u64; i++) {
            log_errorf("%2.2x", *((unsigned char *)ctx->args[0].value.ptr + i));
        }
        log_errorf("]\n");
    }

    if (!EQUAL_SEED) {
        REPORT("%lu", _, seed, enforce_state()->seed, prng_seed());
    }
}

void
_save(const context_t *ctx, const event_t *e)
{
    switch (ctx->cat) {
        case CAT_BEFORE_READ:
        case CAT_BEFORE_AREAD:
        case CAT_BEFORE_WRITE:
        case CAT_BEFORE_AWRITE:
            if (MODE(DATA)) {
                enforce_state()->val =
                    _read_val(&ctx->args[0], ctx->args[1].value.u64);
            }
            break;

        case CAT_ENFORCE:
            if (!MODE(CUSTOM)) {
                break;
            }
            ASSERT(ctx->args[0].width == ARG_PTR);
            ASSERT(ctx->args[1].value.u64 <= ENFORCE_DATA_SIZE);
            sys_memcpy(enforce_state()->data, (char *)ctx->args[0].value.ptr,
                       ctx->args[1].value.u64);
            sys_memset(enforce_state()->data + ctx->args[1].value.u64, 0,
                       ENFORCE_DATA_SIZE - ctx->args[1].value.u64);
            break;

        default:
            break;
    }
    enforce_state()->clk = e->clk;
    if (MODE(CAT) || MODE(TID) || MODE(ADDRESS)) {
        enforce_state()->ctx = *ctx;
    }
    if (MODE(PC)) {
        enforce_state()->pc = stable_address_get(
            ctx->pc, sequencer_config()->stable_address_method);
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
        log_warnf("Can't read ASLR status\n");
    } else {
        char aslr[1];
        sys_fread(aslr, 1, 1, fp);
        if (aslr[0] != '0' &&
            (MODE(PC) || MODE(ADDRESS) || MODE(DATA) || MODE(CUSTOM)) &&
            sequencer_config()->stable_address_method ==
                STABLE_ADDRESS_METHOD_NONE) {
            log_warnf("ASLR enabled but no stable address method\n");
        }
    }
}

void
_handle(const context_t *ctx, event_t *cp)
{
    once(check_aslr());
    if (enforce_config()->modes == ENFORCE_MODE_NONE)
        return;
    if (MODE(CUSTOM) && ctx->cat == CAT_ENFORCE) {
        cp->should_record = true;
    }
    if (cp->replay && cp->clk == enforce_state()->clk) {
        if (!_as_expected(ctx)) {
            log_errorf(
                "Replay mismatch! cappt = [clk: %lu, id: %lu, cat: %s, pc: "
                "%p]\n",
                cp->clk, ctx->id, category_str(ctx->cat), (void *)ctx->pc);
            _report(ctx);
            log_fatalf("unexpected capture point\n");
            sys_abort();
        }
    }
    _save(ctx, cp);
}
REGISTER_HANDLER(SLOT_ENFORCEMENT, _handle)
