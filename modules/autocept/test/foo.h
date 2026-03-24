/**
 * @file foo.h
 * @brief Corner-case ABI surface used by autocept tests.
 */
#ifndef LOTTO_MODULES_AUTOCEPT_TEST_FOO_H
#define LOTTO_MODULES_AUTOCEPT_TEST_FOO_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#define AUTOCEPT_TEST_FOO_3_SRC_SIZE 32

struct foo_pair {
    uint64_t lo;
    int64_t hi;
};

struct foo_big {
    uint64_t a;
    uint64_t b;
    uint64_t c;
};

struct foo_huge {
    uint64_t a;
    uint64_t b;
    uint64_t c;
    uint64_t d;
    uint64_t e;
};

int32_t foo_1(int32_t a, uint64_t b, unsigned c);
uintptr_t foo_2(void *ptr, size_t len, int *status);
void *foo_3(const char *src, unsigned *count, intptr_t delta);
uint64_t foo_4(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
               uint64_t a5);
uint64_t foo_5(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
               uint64_t a5, uint64_t a6, uint64_t a7);
struct foo_pair foo_6(struct foo_pair pair, uintptr_t tag);
struct foo_big foo_7(struct foo_big big, uintptr_t tag);
double foo_8(double left, double right, float scale);
uint64_t foo_9(uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,
               uint64_t a5, uint64_t a6, uint64_t a7, uint64_t a8, uint64_t a9,
               uint64_t a10, uint64_t a11);
struct foo_huge foo_10(struct foo_huge huge, uintptr_t tag, uint32_t salt);
int64_t foo_11(int8_t a, uint16_t b, int32_t c, uint64_t d, intptr_t e,
               size_t f, int8_t g, uint16_t h, int64_t i);
void foo_12(uint32_t *out0, uint64_t *out1, const void *ptr, size_t len,
            int flag);
#ifndef AUTOCEPT_TEST_MOCKOTO
uint64_t foo_13(unsigned count, ...);
double foo_14(unsigned count, ...);
#endif

struct foo_1_event {
    int32_t a;
    uint64_t b;
    unsigned c;
    int32_t ret;
};

struct foo_2_event {
    void *ptr;
    size_t len;
    int status_in;
    int status_out;
    uintptr_t ret;
};

struct foo_3_event {
    char src[AUTOCEPT_TEST_FOO_3_SRC_SIZE];
    unsigned count_in;
    unsigned count_out;
    intptr_t delta;
    void *ret;
};

struct foo_4_event {
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t ret;
};

struct foo_5_event {
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t ret;
};

struct foo_6_event {
    struct foo_pair pair;
    uintptr_t tag;
    struct foo_pair ret;
};

struct foo_7_event {
    struct foo_big big;
    uintptr_t tag;
    struct foo_big ret;
};

struct foo_8_event {
    double left;
    double right;
    float scale;
    double ret;
};

struct foo_9_event {
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t a4;
    uint64_t a5;
    uint64_t a6;
    uint64_t a7;
    uint64_t a8;
    uint64_t a9;
    uint64_t a10;
    uint64_t a11;
    uint64_t ret;
};

struct foo_10_event {
    struct foo_huge huge;
    uintptr_t tag;
    uint32_t salt;
    struct foo_huge ret;
};

struct foo_11_event {
    int8_t a;
    uint16_t b;
    int32_t c;
    uint64_t d;
    intptr_t e;
    size_t f;
    int8_t g;
    uint16_t h;
    int64_t i;
    int64_t ret;
};

struct foo_12_event {
    uint32_t out0_init;
    uint64_t out1_init;
    const void *ptr;
    size_t len;
    int flag;
    uint32_t out0_final;
    uint64_t out1_final;
};

#ifndef AUTOCEPT_TEST_MOCKOTO
struct foo_13_event {
    unsigned count;
    uint64_t a0;
    uint64_t a1;
    uint64_t a2;
    uint64_t a3;
    uint64_t ret;
};

struct foo_14_event {
    unsigned count;
    double a0;
    double a1;
    double a2;
    double ret;
};
#endif

#endif
