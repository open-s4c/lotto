#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "autocept_mock.h"
#include "foo.h"
#include "variadic_mock.h"

typedef struct {
    uint64_t seed;
    size_t iter;
    const char *func;
} test_context_t;

struct test_case {
    const char *name;
    void (*run)(void);
    bool stable;
};

static test_context_t _ctx;
static uint64_t _rng_state;

static void
_fail_check(const char *expr, int line)
{
    fprintf(stderr,
            "autocept baseline failure: %s at line %d (seed=%" PRIu64
            ", iter=%zu, func=%s)\n",
            expr, line, _ctx.seed, _ctx.iter, _ctx.func);
    abort();
}

#define CHECK(expr)                                                            \
    do {                                                                       \
        if (!(expr))                                                           \
            _fail_check(#expr, __LINE__);                                      \
    } while (0)

static void
_start_test(const char *name)
{
    printf("Test %s\n", name);
    fflush(stdout);
}

static void
_report_ok(const char *stage)
{
    printf("  %s OK\n", stage);
    fflush(stdout);
}

static uint64_t
_rng_next(void)
{
    _rng_state = _rng_state * UINT64_C(6364136223846793005) + UINT64_C(1);
    return _rng_state;
}

static unsigned
_rng_u32(void)
{
    return (unsigned)(_rng_next() >> 32);
}

static int32_t
_rng_i32(void)
{
    return (int32_t)_rng_u32();
}

static intptr_t
_rng_iptr(void)
{
    return (intptr_t)_rng_next();
}

static double
_rng_f64(void)
{
    return (double)((int32_t)(_rng_next() >> 32)) / 257.0;
}

static float
_rng_f32(void)
{
    return (float)((int32_t)(_rng_next() >> 40)) / 31.0f;
}

static void
_rng_fill_bytes(void *dst, size_t size)
{
    unsigned char *bytes = dst;

    for (size_t i = 0; i < size; i++) {
        bytes[i] = (unsigned char)_rng_u32();
    }
}

static void
_rng_fill_string(char *dst, size_t size)
{
    CHECK(size > 1);
    size_t len = (_rng_u32() % (size - 1)) + 1;

    for (size_t i = 0; i < len; i++) {
        dst[i] = (char)('a' + (_rng_u32() % 26));
    }
    dst[len] = '\0';
}

static struct foo_pair
_rng_pair(void)
{
    struct foo_pair pair;

    _rng_fill_bytes(&pair, sizeof(pair));
    return pair;
}

static struct foo_big
_rng_big(void)
{
    struct foo_big big;

    _rng_fill_bytes(&big, sizeof(big));
    return big;
}

static struct foo_huge
_rng_huge(void)
{
    struct foo_huge huge;

    _rng_fill_bytes(&huge, sizeof(huge));
    return huge;
}

static bool
_pair_eq(struct foo_pair lhs, struct foo_pair rhs)
{
    return lhs.lo == rhs.lo && lhs.hi == rhs.hi;
}

static bool
_big_eq(struct foo_big lhs, struct foo_big rhs)
{
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c;
}

static bool
_huge_eq(struct foo_huge lhs, struct foo_huge rhs)
{
    return lhs.a == rhs.a && lhs.b == rhs.b && lhs.c == rhs.c &&
           lhs.d == rhs.d && lhs.e == rhs.e;
}

static unsigned
_iters(void)
{
    const char *var = getenv("AUTOCEPT_TEST_ITERS");
    if (var == NULL || *var == '\0')
        return 16;

    return (unsigned)strtoul(var, NULL, 0);
}

/* foo_1 */
static const struct foo_1_event *_foo_1_expected;

static int32_t
_foo_1_hook(int32_t a, uint64_t b, unsigned c)
{
    CHECK(_foo_1_expected != NULL);
    CHECK(a == _foo_1_expected->a);
    CHECK(b == _foo_1_expected->b);
    CHECK(c == _foo_1_expected->c);
    _report_ok("mock  ");
    return _foo_1_expected->ret;
}

static struct foo_1_event
_foo_1_random_event(void)
{
    return (struct foo_1_event){
        .a   = _rng_i32(),
        .b   = _rng_next(),
        .c   = _rng_u32(),
        .ret = _rng_i32(),
    };
}

static void
test_foo_1(void)
{
    struct foo_1_event ev = _foo_1_random_event();
    _ctx.func             = "foo_1";
    _foo_1_expected       = &ev;
    _start_test("foo_1");
    mockoto_foo_1_hook(_foo_1_hook);

    int32_t ret = foo_1(ev.a, ev.b, ev.c);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_1_called() == 1);
    _report_ok("caller");

    _foo_1_expected = NULL;
}

/* foo_2 */
static const struct foo_2_event *_foo_2_expected;

static uintptr_t
_foo_2_hook(void *ptr, size_t len, int *status)
{
    CHECK(_foo_2_expected != NULL);
    CHECK(ptr == _foo_2_expected->ptr);
    CHECK(len == _foo_2_expected->len);
    CHECK(status != NULL);
    CHECK(*status == _foo_2_expected->status_in);
    *status = _foo_2_expected->status_out;
    _report_ok("mock  ");
    return _foo_2_expected->ret;
}

static struct foo_2_event
_foo_2_random_event(void)
{
    return (struct foo_2_event){
        .ptr        = (void *)(uintptr_t)(_rng_next() | UINT64_C(1)),
        .len        = (size_t)(_rng_u32() % 4096),
        .status_in  = _rng_i32(),
        .status_out = _rng_i32(),
        .ret        = (uintptr_t)_rng_next(),
    };
}

static void
test_foo_2(void)
{
    struct foo_2_event ev = _foo_2_random_event();
    _ctx.func             = "foo_2";
    _foo_2_expected       = &ev;
    _start_test("foo_2");
    mockoto_foo_2_hook(_foo_2_hook);

    int status    = ev.status_in;
    uintptr_t ret = foo_2(ev.ptr, ev.len, &status);
    CHECK(status == ev.status_out);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_2_called() == 1);
    _report_ok("caller");

    _foo_2_expected = NULL;
}

/* foo_3 */
static const struct foo_3_event *_foo_3_expected;

static void *
_foo_3_hook(const char *src, unsigned *count, intptr_t delta)
{
    CHECK(_foo_3_expected != NULL);
    CHECK(src != NULL);
    CHECK(strcmp(src, _foo_3_expected->src) == 0);
    CHECK(count != NULL);
    CHECK(*count == _foo_3_expected->count_in);
    CHECK(delta == _foo_3_expected->delta);
    *count = _foo_3_expected->count_out;
    _report_ok("mock  ");
    return _foo_3_expected->ret;
}

static struct foo_3_event
_foo_3_random_event(void)
{
    struct foo_3_event ev = {
        .count_in  = _rng_u32(),
        .count_out = _rng_u32(),
        .delta     = _rng_iptr(),
        .ret       = (void *)(uintptr_t)(_rng_next() | UINT64_C(1)),
    };

    _rng_fill_string(ev.src, sizeof(ev.src));
    return ev;
}

static void
test_foo_3(void)
{
    struct foo_3_event ev = _foo_3_random_event();
    _ctx.func             = "foo_3";
    _foo_3_expected       = &ev;
    _start_test("foo_3");
    mockoto_foo_3_hook(_foo_3_hook);

    unsigned count = ev.count_in;
    void *ret      = foo_3(ev.src, &count, ev.delta);
    CHECK(count == ev.count_out);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_3_called() == 1);
    _report_ok("caller");

    _foo_3_expected = NULL;
}

/* foo_4 */
static const struct foo_4_event *_foo_4_expected;

static uint64_t
_foo_4_hook(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
            uint64_t a5)
{
    CHECK(_foo_4_expected != NULL);
    CHECK(a0 == _foo_4_expected->a0);
    CHECK(a1 == _foo_4_expected->a1);
    CHECK(a2 == _foo_4_expected->a2);
    CHECK(a3 == _foo_4_expected->a3);
    CHECK(a4 == _foo_4_expected->a4);
    CHECK(a5 == _foo_4_expected->a5);
    _report_ok("mock  ");
    return _foo_4_expected->ret;
}

static struct foo_4_event
_foo_4_random_event(void)
{
    return (struct foo_4_event){
        .a0  = _rng_next(),
        .a1  = _rng_next(),
        .a2  = _rng_next(),
        .a3  = _rng_next(),
        .a4  = _rng_next(),
        .a5  = _rng_next(),
        .ret = _rng_next(),
    };
}

static void
test_foo_4(void)
{
    struct foo_4_event ev = _foo_4_random_event();
    _ctx.func             = "foo_4";
    _foo_4_expected       = &ev;
    _start_test("foo_4");
    mockoto_foo_4_hook(_foo_4_hook);

    uint64_t ret = foo_4(ev.a0, ev.a1, ev.a2, ev.a3, ev.a4, ev.a5);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_4_called() == 1);
    _report_ok("caller");

    _foo_4_expected = NULL;
}

/* foo_5 */
static const struct foo_5_event *_foo_5_expected;

static uint64_t
_foo_5_hook(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
            uint64_t a5, uint64_t a6, uint64_t a7)
{
    CHECK(_foo_5_expected != NULL);
    CHECK(a0 == _foo_5_expected->a0);
    CHECK(a1 == _foo_5_expected->a1);
    CHECK(a2 == _foo_5_expected->a2);
    CHECK(a3 == _foo_5_expected->a3);
    CHECK(a4 == _foo_5_expected->a4);
    CHECK(a5 == _foo_5_expected->a5);
    CHECK(a6 == _foo_5_expected->a6);
    CHECK(a7 == _foo_5_expected->a7);
    _report_ok("mock  ");
    return _foo_5_expected->ret;
}

static struct foo_5_event
_foo_5_random_event(void)
{
    return (struct foo_5_event){
        .a0  = _rng_next(),
        .a1  = _rng_next(),
        .a2  = _rng_next(),
        .a3  = _rng_next(),
        .a4  = _rng_next(),
        .a5  = _rng_next(),
        .a6  = _rng_next(),
        .a7  = _rng_next(),
        .ret = _rng_next(),
    };
}

static void
test_foo_5(void)
{
    struct foo_5_event ev = _foo_5_random_event();
    _ctx.func             = "foo_5";
    _foo_5_expected       = &ev;
    _start_test("foo_5");
    mockoto_foo_5_hook(_foo_5_hook);

    uint64_t ret =
        foo_5(ev.a0, ev.a1, ev.a2, ev.a3, ev.a4, ev.a5, ev.a6, ev.a7);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_5_called() == 1);
    _report_ok("caller");

    _foo_5_expected = NULL;
}

/* foo_6 */
static const struct foo_6_event *_foo_6_expected;

static struct foo_pair
_foo_6_hook(struct foo_pair pair, uintptr_t tag)
{
    CHECK(_foo_6_expected != NULL);
    CHECK(_pair_eq(pair, _foo_6_expected->pair));
    CHECK(tag == _foo_6_expected->tag);
    _report_ok("mock  ");
    return _foo_6_expected->ret;
}

static struct foo_6_event
_foo_6_random_event(void)
{
    return (struct foo_6_event){
        .pair = _rng_pair(),
        .tag  = (uintptr_t)_rng_next(),
        .ret  = _rng_pair(),
    };
}

static void
test_foo_6(void)
{
    struct foo_6_event ev = _foo_6_random_event();
    _ctx.func             = "foo_6";
    _foo_6_expected       = &ev;
    _start_test("foo_6");
    mockoto_foo_6_hook(_foo_6_hook);

    struct foo_pair ret = foo_6(ev.pair, ev.tag);
    CHECK(_pair_eq(ret, ev.ret));
    CHECK(mockoto_foo_6_called() == 1);
    _report_ok("caller");

    _foo_6_expected = NULL;
}

/* foo_7 */
static const struct foo_7_event *_foo_7_expected;

static struct foo_big
_foo_7_hook(struct foo_big big, uintptr_t tag)
{
    CHECK(_foo_7_expected != NULL);
    CHECK(_big_eq(big, _foo_7_expected->big));
    CHECK(tag == _foo_7_expected->tag);
    _report_ok("mock  ");
    return _foo_7_expected->ret;
}

static struct foo_7_event
_foo_7_random_event(void)
{
    return (struct foo_7_event){
        .big = _rng_big(),
        .tag = (uintptr_t)_rng_next(),
        .ret = _rng_big(),
    };
}

static void
test_foo_7(void)
{
    struct foo_7_event ev = _foo_7_random_event();
    _ctx.func             = "foo_7";
    _foo_7_expected       = &ev;
    _start_test("foo_7");
    mockoto_foo_7_hook(_foo_7_hook);

    struct foo_big ret = foo_7(ev.big, ev.tag);
    CHECK(_big_eq(ret, ev.ret));
    CHECK(mockoto_foo_7_called() == 1);
    _report_ok("caller");

    _foo_7_expected = NULL;
}

/* foo_8 */
static const struct foo_8_event *_foo_8_expected;

static double
_foo_8_hook(double left, double right, float scale)
{
    CHECK(_foo_8_expected != NULL);
    CHECK(left == _foo_8_expected->left);
    CHECK(right == _foo_8_expected->right);
    CHECK(scale == _foo_8_expected->scale);
    _report_ok("mock  ");
    return _foo_8_expected->ret;
}

static struct foo_8_event
_foo_8_random_event(void)
{
    return (struct foo_8_event){
        .left  = _rng_f64(),
        .right = _rng_f64(),
        .scale = _rng_f32(),
        .ret   = _rng_f64(),
    };
}

static void
test_foo_8(void)
{
    struct foo_8_event ev = _foo_8_random_event();
    _ctx.func             = "foo_8";
    _foo_8_expected       = &ev;
    _start_test("foo_8");
    mockoto_foo_8_hook(_foo_8_hook);

    double ret = foo_8(ev.left, ev.right, ev.scale);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_8_called() == 1);
    _report_ok("caller");

    _foo_8_expected = NULL;
}

/* foo_9 */
static const struct foo_9_event *_foo_9_expected;

static uint64_t
_foo_9_hook(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
            uint64_t a5, uint64_t a6, uint64_t a7, uint64_t a8, uint64_t a9,
            uint64_t a10, uint64_t a11)
{
    CHECK(_foo_9_expected != NULL);
    CHECK(a0 == _foo_9_expected->a0);
    CHECK(a1 == _foo_9_expected->a1);
    CHECK(a2 == _foo_9_expected->a2);
    CHECK(a3 == _foo_9_expected->a3);
    CHECK(a4 == _foo_9_expected->a4);
    CHECK(a5 == _foo_9_expected->a5);
    CHECK(a6 == _foo_9_expected->a6);
    CHECK(a7 == _foo_9_expected->a7);
    CHECK(a8 == _foo_9_expected->a8);
    CHECK(a9 == _foo_9_expected->a9);
    CHECK(a10 == _foo_9_expected->a10);
    CHECK(a11 == _foo_9_expected->a11);
    _report_ok("mock  ");
    return _foo_9_expected->ret;
}

static struct foo_9_event
_foo_9_random_event(void)
{
    return (struct foo_9_event){
        .a0  = _rng_next(),
        .a1  = _rng_next(),
        .a2  = _rng_next(),
        .a3  = _rng_next(),
        .a4  = _rng_next(),
        .a5  = _rng_next(),
        .a6  = _rng_next(),
        .a7  = _rng_next(),
        .a8  = _rng_next(),
        .a9  = _rng_next(),
        .a10 = _rng_next(),
        .a11 = _rng_next(),
        .ret = _rng_next(),
    };
}

static void
test_foo_9(void)
{
    struct foo_9_event ev = _foo_9_random_event();
    _ctx.func             = "foo_9";
    _foo_9_expected       = &ev;
    _start_test("foo_9");
    mockoto_foo_9_hook(_foo_9_hook);

    uint64_t ret = foo_9(ev.a0, ev.a1, ev.a2, ev.a3, ev.a4, ev.a5, ev.a6, ev.a7,
                         ev.a8, ev.a9, ev.a10, ev.a11);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_9_called() == 1);
    _report_ok("caller");

    _foo_9_expected = NULL;
}

/* foo_10 */
static const struct foo_10_event *_foo_10_expected;

static struct foo_huge
_foo_10_hook(struct foo_huge huge, uintptr_t tag, uint32_t salt)
{
    CHECK(_foo_10_expected != NULL);
    CHECK(_huge_eq(huge, _foo_10_expected->huge));
    CHECK(tag == _foo_10_expected->tag);
    CHECK(salt == _foo_10_expected->salt);
    _report_ok("mock  ");
    return _foo_10_expected->ret;
}

static struct foo_10_event
_foo_10_random_event(void)
{
    return (struct foo_10_event){
        .huge = _rng_huge(),
        .tag  = (uintptr_t)_rng_next(),
        .salt = _rng_u32(),
        .ret  = _rng_huge(),
    };
}

static void
test_foo_10(void)
{
    struct foo_10_event ev = _foo_10_random_event();
    _ctx.func              = "foo_10";
    _foo_10_expected       = &ev;
    _start_test("foo_10");
    mockoto_foo_10_hook(_foo_10_hook);

    struct foo_huge ret = foo_10(ev.huge, ev.tag, ev.salt);
    CHECK(_huge_eq(ret, ev.ret));
    CHECK(mockoto_foo_10_called() == 1);
    _report_ok("caller");

    _foo_10_expected = NULL;
}

/* foo_11 */
static const struct foo_11_event *_foo_11_expected;

static int64_t
_foo_11_hook(int8_t a, uint16_t b, int32_t c, uint64_t d, intptr_t e, size_t f,
             int8_t g, uint16_t h, int64_t i)
{
    CHECK(_foo_11_expected != NULL);
    CHECK(a == _foo_11_expected->a);
    CHECK(b == _foo_11_expected->b);
    CHECK(c == _foo_11_expected->c);
    CHECK(d == _foo_11_expected->d);
    CHECK(e == _foo_11_expected->e);
    CHECK(f == _foo_11_expected->f);
    CHECK(g == _foo_11_expected->g);
    CHECK(h == _foo_11_expected->h);
    CHECK(i == _foo_11_expected->i);
    _report_ok("mock  ");
    return _foo_11_expected->ret;
}

static struct foo_11_event
_foo_11_random_event(void)
{
    return (struct foo_11_event){
        .a   = (int8_t)_rng_u32(),
        .b   = (uint16_t)_rng_u32(),
        .c   = _rng_i32(),
        .d   = _rng_next(),
        .e   = _rng_iptr(),
        .f   = (size_t)_rng_u32(),
        .g   = (int8_t)_rng_u32(),
        .h   = (uint16_t)_rng_u32(),
        .i   = (int64_t)_rng_next(),
        .ret = (int64_t)_rng_next(),
    };
}

static void
test_foo_11(void)
{
    struct foo_11_event ev = _foo_11_random_event();
    _ctx.func              = "foo_11";
    _foo_11_expected       = &ev;
    _start_test("foo_11");
    mockoto_foo_11_hook(_foo_11_hook);

    int64_t ret = foo_11(ev.a, ev.b, ev.c, ev.d, ev.e, ev.f, ev.g, ev.h, ev.i);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_11_called() == 1);
    _report_ok("caller");

    _foo_11_expected = NULL;
}

/* foo_12 */
static const struct foo_12_event *_foo_12_expected;

static void
_foo_12_hook(uint32_t *out0, uint64_t *out1, const void *ptr, size_t len,
             int flag)
{
    CHECK(_foo_12_expected != NULL);
    CHECK(out0 != NULL);
    CHECK(out1 != NULL);
    CHECK(*out0 == _foo_12_expected->out0_init);
    CHECK(*out1 == _foo_12_expected->out1_init);
    CHECK(ptr == _foo_12_expected->ptr);
    CHECK(len == _foo_12_expected->len);
    CHECK(flag == _foo_12_expected->flag);
    *out0 = _foo_12_expected->out0_final;
    *out1 = _foo_12_expected->out1_final;
    _report_ok("mock  ");
}

static struct foo_12_event
_foo_12_random_event(void)
{
    return (struct foo_12_event){
        .out0_init  = _rng_u32(),
        .out1_init  = _rng_next(),
        .ptr        = (const void *)(uintptr_t)(_rng_next() | UINT64_C(1)),
        .len        = (size_t)(_rng_u32() % 8192),
        .flag       = _rng_i32(),
        .out0_final = _rng_u32(),
        .out1_final = _rng_next(),
    };
}

static void
test_foo_12(void)
{
    struct foo_12_event ev = _foo_12_random_event();
    _ctx.func              = "foo_12";
    _foo_12_expected       = &ev;
    _start_test("foo_12");
    mockoto_foo_12_hook(_foo_12_hook);

    uint32_t out0 = ev.out0_init;
    uint64_t out1 = ev.out1_init;
    foo_12(&out0, &out1, ev.ptr, ev.len, ev.flag);
    CHECK(out0 == ev.out0_final);
    CHECK(out1 == ev.out1_final);
    CHECK(mockoto_foo_12_called() == 1);
    _report_ok("caller");

    _foo_12_expected = NULL;
}

/* foo_13 */
static const struct foo_13_event *_foo_13_expected;

static uint64_t
_foo_13_hook(unsigned count, va_list ap)
{
    CHECK(_foo_13_expected != NULL);
    CHECK(count == _foo_13_expected->count);
    CHECK(va_arg(ap, uint64_t) == _foo_13_expected->a0);
    CHECK(va_arg(ap, uint64_t) == _foo_13_expected->a1);
    CHECK(va_arg(ap, uint64_t) == _foo_13_expected->a2);
    CHECK(va_arg(ap, uint64_t) == _foo_13_expected->a3);
    _report_ok("mock  ");
    return _foo_13_expected->ret;
}

static struct foo_13_event
_foo_13_random_event(void)
{
    return (struct foo_13_event){
        .count = 4,
        .a0    = _rng_next(),
        .a1    = _rng_next(),
        .a2    = _rng_next(),
        .a3    = _rng_next(),
        .ret   = _rng_next(),
    };
}

static void
test_foo_13(void)
{
    struct foo_13_event ev = _foo_13_random_event();
    _ctx.func              = "foo_13";
    _foo_13_expected       = &ev;
    _start_test("foo_13");
    mockoto_foo_13_hook(_foo_13_hook);

    uint64_t ret = foo_13(ev.count, ev.a0, ev.a1, ev.a2, ev.a3);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_13_called() == 1);
    _report_ok("caller");

    _foo_13_expected = NULL;
}

/* foo_14 */
static const struct foo_14_event *_foo_14_expected;

static double
_foo_14_hook(unsigned count, va_list ap)
{
    CHECK(_foo_14_expected != NULL);
    CHECK(count == _foo_14_expected->count);
    CHECK(va_arg(ap, double) == _foo_14_expected->a0);
    CHECK(va_arg(ap, double) == _foo_14_expected->a1);
    CHECK(va_arg(ap, double) == _foo_14_expected->a2);
    _report_ok("mock  ");
    return _foo_14_expected->ret;
}

static struct foo_14_event
_foo_14_random_event(void)
{
    return (struct foo_14_event){
        .count = 3,
        .a0    = _rng_f64(),
        .a1    = _rng_f64(),
        .a2    = _rng_f64(),
        .ret   = _rng_f64(),
    };
}

static void
test_foo_14(void)
{
    struct foo_14_event ev = _foo_14_random_event();
    _ctx.func              = "foo_14";
    _foo_14_expected       = &ev;
    _start_test("foo_14");
    mockoto_foo_14_hook(_foo_14_hook);

    double ret = foo_14(ev.count, ev.a0, ev.a1, ev.a2);
    CHECK(ret == ev.ret);
    CHECK(mockoto_foo_14_called() == 1);
    _report_ok("caller");

    _foo_14_expected = NULL;
}

static const struct test_case _cases[] = {
    {.name = "foo_1", .run = test_foo_1, .stable = true},
    {.name = "foo_2", .run = test_foo_2, .stable = true},
    {.name = "foo_3", .run = test_foo_3, .stable = true},
    {.name = "foo_4", .run = test_foo_4, .stable = true},
    {.name = "foo_5", .run = test_foo_5, .stable = true},
    {.name = "foo_6", .run = test_foo_6, .stable = true},
    {.name = "foo_7", .run = test_foo_7, .stable = true},
    {.name = "foo_8", .run = test_foo_8, .stable = true},
    {.name = "foo_9", .run = test_foo_9, .stable = true},
    {.name = "foo_10", .run = test_foo_10, .stable = true},
    {.name = "foo_11", .run = test_foo_11, .stable = true},
    {.name = "foo_12", .run = test_foo_12, .stable = true},
    {.name = "foo_13", .run = test_foo_13, .stable = true},
    {.name = "foo_14", .run = test_foo_14, .stable = true},
};

int
main(int argc, char **argv)
{
    const char *filter = getenv("AUTOCEPT_TEST_CASE");
    const char *mode   = getenv("AUTOCEPT_TEST_MODE");
    bool run_all       = mode != NULL && strcmp(mode, "all") == 0;
    unsigned iters     = _iters();
    bool ran_any       = false;

    if (argc > 2) {
        fprintf(stderr, "usage: %s [seed]\n", argv[0]);
        return 2;
    }

    _ctx.seed  = argc == 2 ? strtoull(argv[1], NULL, 0) : UINT64_C(0x4f3c2b1a);
    _rng_state = _ctx.seed;

    for (size_t i = 0; i < sizeof(_cases) / sizeof(_cases[0]); i++) {
        const struct test_case *tc = &_cases[i];

        if (filter != NULL && strcmp(filter, tc->name) != 0)
            continue;
        if (filter == NULL && !run_all && !tc->stable)
            continue;

        for (_ctx.iter = 0; _ctx.iter < iters; _ctx.iter++) {
            tc->run();
            ran_any = true;
        }
    }

    CHECK(ran_any);
    return 0;
}
