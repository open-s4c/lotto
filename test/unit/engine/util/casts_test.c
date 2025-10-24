#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <lotto/util/casts.h>

bool rb;
int8_t ri8;
uint8_t ru8;
int16_t ri16;
uint16_t ru16;
int32_t ri32;
uint32_t ru32;
int64_t ri64;
uint64_t ru64;
size_t rs;

static bool rsb;
static int8_t rsi8;
static uint8_t rsu8;
static int16_t rsi16;
static uint16_t rsu16;
static int32_t rsi32;
static uint32_t rsu32;
static int64_t rsi64;
static uint64_t rsu64;
static size_t rss;

const bool rcb;
const int8_t rci8;
const uint8_t rcu8;
const int16_t rci16;
const uint16_t rcu16;
const int32_t rci32;
const uint32_t rcu32;
const int64_t rci64;
const uint64_t rcu64;
const size_t rcs;

#if defined(__clang__)
_Pragma("clang diagnostic push")
    _Pragma("clang diagnostic ignored \"-Wsign-compare\"")
#elif defined(__GNUC__)
_Pragma("GCC diagnostic push")
    _Pragma("GCC diagnostic ignored \"-Wsign-compare\"")
#else
    #warning "Unknown compiler"
#endif

        void helper_test()
{
    rsb   = 0;
    rsi8  = 0;
    rsu8  = 0;
    rsi16 = 0;
    rsu16 = 0;
    rsi32 = 0;
    rsu32 = 0;
    rsi64 = 0;
    rsu64 = 0;
    rss   = 0;

    // check signedness
    ASSERT(ISUNSIGNED_TYPE(bool));
    ASSERT(ISUNSIGNED_VAR(rb));
    ASSERT(ISUNSIGNED_VAR(rsb));
    ASSERT(ISUNSIGNED_VAR(rcb));

    // check signedness
    ASSERT(!ISUNSIGNED_TYPE(int8_t));
    ASSERT(!ISUNSIGNED_VAR(ri8));
    ASSERT(!ISUNSIGNED_VAR(rsi8));
    ASSERT(!ISUNSIGNED_VAR(rci8));

    // check signedness
    ASSERT(ISUNSIGNED_TYPE(uint8_t));
    ASSERT(ISUNSIGNED_VAR(ru8));
    ASSERT(ISUNSIGNED_VAR(rsu8));
    ASSERT(ISUNSIGNED_VAR(rcu8));

    // check signedness
    ASSERT(!ISUNSIGNED_TYPE(int16_t));
    ASSERT(!ISUNSIGNED_VAR(ri16));
    ASSERT(!ISUNSIGNED_VAR(rsi16));
    ASSERT(!ISUNSIGNED_VAR(rci16));

    // check signedness
    ASSERT(ISUNSIGNED_TYPE(uint16_t));
    ASSERT(ISUNSIGNED_VAR(ru16));
    ASSERT(ISUNSIGNED_VAR(rsu16));
    ASSERT(ISUNSIGNED_VAR(rcu16));

    // check signedness
    ASSERT(!ISUNSIGNED_TYPE(int32_t));
    ASSERT(!ISUNSIGNED_VAR(ri32));
    ASSERT(!ISUNSIGNED_VAR(rsi32));
    ASSERT(!ISUNSIGNED_VAR(rci32));

    // check signedness
    ASSERT(ISUNSIGNED_TYPE(uint32_t));
    ASSERT(ISUNSIGNED_VAR(ru32));
    ASSERT(ISUNSIGNED_VAR(rsu32));
    ASSERT(ISUNSIGNED_VAR(rcu32));

    // check signedness
    ASSERT(!ISUNSIGNED_TYPE(int64_t));
    ASSERT(!ISUNSIGNED_VAR(ri64));
    ASSERT(!ISUNSIGNED_VAR(rsi64));
    ASSERT(!ISUNSIGNED_VAR(rci64));

    // check signedness
    ASSERT(ISUNSIGNED_TYPE(uint64_t));
    ASSERT(ISUNSIGNED_VAR(ru64));
    ASSERT(ISUNSIGNED_VAR(rsu64));
    ASSERT(ISUNSIGNED_VAR(rcu64));

    // check signedness
    ASSERT(ISUNSIGNED_TYPE(size_t));
    ASSERT(ISUNSIGNED_VAR(rs));
    ASSERT(ISUNSIGNED_VAR(rss));
    ASSERT(ISUNSIGNED_VAR(rcs));

    // check MIN/MAX of type
    printf("(int8_t) min: %ld vs. %d max: %ld vs. %d\n", TYPE_MIN(int8_t),
           INT8_MIN, TYPE_MAX(int8_t), INT8_MAX);
    ASSERT(TYPE_MIN(int8_t) == INT8_MIN && TYPE_MAX(int8_t) == INT8_MAX);

    // check MIN/MAX of type
    printf("(uint8_t) min: %lu vs. %u max: %lu vs. %u\n", TYPE_MIN(uint8_t), 0,
           TYPE_MAX(uint8_t), UINT8_MAX);
    ASSERT(TYPE_MIN(uint8_t) == 0 && TYPE_MAX(uint8_t) == UINT8_MAX);

    // check MIN/MAX of type
    printf("(int16_t) min: %ld vs. %d max: %ld vs. %d\n", TYPE_MIN(int16_t),
           INT16_MIN, TYPE_MAX(int16_t), INT16_MAX);
    ASSERT(TYPE_MIN(int16_t) == INT16_MIN && TYPE_MAX(int16_t) == INT16_MAX);

    // check MIN/MAX of type
    printf("(uint16_t) min: %lu vs. %u max: %lu vs. %u\n", TYPE_MIN(uint16_t),
           0, TYPE_MAX(uint16_t), UINT16_MAX);
    ASSERT(TYPE_MIN(uint16_t) == 0 && TYPE_MAX(uint16_t) == UINT16_MAX);

    // check MIN/MAX of type
    printf("(int32_t) min: %ld vs. %d max: %ld vs. %d\n", TYPE_MIN(int32_t),
           INT32_MIN, TYPE_MAX(int32_t), INT32_MAX);
    ASSERT(TYPE_MIN(int32_t) == INT32_MIN && TYPE_MAX(int32_t) == INT32_MAX);

    // check MIN/MAX of type
    printf("(uint32_t) min: %lu vs. %u max: %lu vs. %u\n", TYPE_MIN(uint32_t),
           0, TYPE_MAX(uint32_t), UINT32_MAX);
    ASSERT(TYPE_MIN(uint32_t) == 0 && TYPE_MAX(uint32_t) == UINT32_MAX);

    // check MIN/MAX of type
    printf("(int64_t) min: %ld vs. %ld max: %ld vs. %ld\n", TYPE_MIN(int64_t),
           INT64_MIN, TYPE_MAX(int64_t), INT64_MAX);
    ASSERT(TYPE_MIN(int64_t) == INT64_MIN && TYPE_MAX(int64_t) == INT64_MAX);

    // check MIN/MAX of type
    printf("(uint64_t) min: %lu vs. %lu max: %lu vs. %lu\n", TYPE_MIN(uint64_t),
           (uint64_t)0, TYPE_MAX(uint64_t), UINT64_MAX);
    ASSERT(TYPE_MIN(uint64_t) == 0 && TYPE_MAX(uint64_t) == UINT64_MAX);
}

#define BOOL_VARIANTS 2
void
bool_test()
{
    printf("Checking conversion from bool\n");
    // all relevant variantes
    bool set[BOOL_VARIANTS] = {false, true};

    // go through all relevant values
    for (uint64_t i = 0; i < BOOL_VARIANTS; i++) {
        printf("Checking value: %s (%u)\n", set[i] == true ? "true" : "false",
               set[i]);

        // check conversions

        // should always succeed for larger types
        rb   = CAST_TYPE(bool, set[i]);
        ri8  = CAST_TYPE(int8_t, set[i]);
        ru8  = CAST_TYPE(uint8_t, set[i]);
        ri16 = CAST_TYPE(int16_t, set[i]);
        ru16 = CAST_TYPE(uint16_t, set[i]);
        ri32 = CAST_TYPE(int32_t, set[i]);
        ru32 = CAST_TYPE(uint32_t, set[i]);
        ri64 = CAST_TYPE(int64_t, set[i]);
        ru64 = CAST_TYPE(uint64_t, set[i]);
        rs   = CAST_TYPE(size_t, set[i]);
    }
}

#define INT8_VARIANTS 5
void
i8_test()
{
    printf("Checking conversion from int8_t\n");
    // all relevant variantes
    int8_t set[INT8_VARIANTS] = {INT8_MIN, -1, 0, 1, INT8_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < INT8_VARIANTS; i++) {
        printf("Checking value: %d\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i >= 2 && i <= 3)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should always succeed to same type
        ri8 = CAST_TYPE(int8_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru8 = CAST_TYPE(uint8_t, set[i]);
        else
            ru8 = CAST_TYPE_FAIL(uint8_t, set[i]);

        // should always succeed for larger types of same signedness
        ri16 = CAST_TYPE(int16_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru16 = CAST_TYPE(uint16_t, set[i]);
        else
            ru16 = CAST_TYPE_FAIL(uint16_t, set[i]);

        // should always succeed for larger types of same signedness
        ri32 = CAST_TYPE(int32_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru32 = CAST_TYPE(uint32_t, set[i]);
        else
            ru32 = CAST_TYPE_FAIL(uint32_t, set[i]);

        // should always succeed for larger types of same signedness
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru64 = CAST_TYPE(uint64_t, set[i]);
        else
            ru64 = CAST_TYPE_FAIL(uint64_t, set[i]);
    }
}

#define UINT8_VARIANTS 4
void
u8_test()
{
    printf("Checking conversion from uint8_t\n");
    // all relevant variantes
    uint8_t set[UINT8_VARIANTS] = {0, 1, INT8_MAX, UINT8_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < UINT8_VARIANTS; i++) {
        printf("Checking value: %d\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i <= 1)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should succeed up to (including) signed max
        if (i <= 2)
            ri8 = CAST_TYPE(int8_t, set[i]);
        else
            ri8 = CAST_TYPE_FAIL(int8_t, set[i]);

        // should always succeed to same type
        ru8 = CAST_TYPE(uint8_t, set[i]);

        // should always succeed for larger signed types
        ri16 = CAST_TYPE(int16_t, set[i]);

        // should always succeed for larger types of same signedness
        ru16 = CAST_TYPE(uint16_t, set[i]);

        // should always succeed for larger signed types
        ri32 = CAST_TYPE(int32_t, set[i]);

        // should always succeed for larger types of same signedness
        ru32 = CAST_TYPE(uint32_t, set[i]);

        // should always succeed for larger signed types
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should always succeed for larger types of same signedness
        ru64 = CAST_TYPE(uint64_t, set[i]);
    }
}

#define INT16_VARIANTS 5
void
i16_test()
{
    printf("Checking conversion from int16_t\n");
    // all relevant variantes
    int16_t set[INT16_VARIANTS] = {INT16_MIN, -1, 0, 1, INT16_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < INT16_VARIANTS; i++) {
        printf("Checking value: %d\n", set[i]);

        // check conversions
        // should succeed for 0 and 1
        if (i >= 2 && i <= 3)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);


        // should only succeed for [target_min,0,target_max]
        if (i >= 1 && i <= 3)
            ri8 = CAST_TYPE(int8_t, set[i]);
        else
            ri8 = CAST_TYPE_FAIL(int8_t, set[i]);

        // should succeed for [0,target_max]
        if (i >= 2 && i <= 3)
            ru8 = CAST_TYPE(uint8_t, set[i]);
        else
            ru8 = CAST_TYPE_FAIL(uint8_t, set[i]);

        // should always succeed to same type
        ri16 = CAST_TYPE(int16_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru16 = CAST_TYPE(uint16_t, set[i]);
        else
            ru16 = CAST_TYPE_FAIL(uint16_t, set[i]);

        // should always succeed for larger types of same signedness
        ri32 = CAST_TYPE(int32_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru32 = CAST_TYPE(uint32_t, set[i]);
        else
            ru32 = CAST_TYPE_FAIL(uint32_t, set[i]);

        // should always succeed for larger types of same signedness
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru64 = CAST_TYPE(uint64_t, set[i]);
        else
            ru64 = CAST_TYPE_FAIL(uint64_t, set[i]);
    }
}

#define UINT16_VARIANTS 4
void
u16_test()
{
    printf("Checking conversion from uint16_t\n");
    // all relevant variantes
    uint16_t set[UINT16_VARIANTS] = {0, 1, INT16_MAX, UINT16_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < UINT8_VARIANTS; i++) {
        printf("Checking value: %d\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i <= 1)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should only succeed for [0,target_max]
        if (i <= 1) {
            ri8 = CAST_TYPE(int8_t, set[i]);
            ru8 = CAST_TYPE(uint8_t, set[i]);
        } else {
            ri8 = CAST_TYPE_FAIL(int8_t, set[i]);
            ru8 = CAST_TYPE_FAIL(uint8_t, set[i]);
        }

        // should succeed up to (including) signed max
        if (i <= 2)
            ri16 = CAST_TYPE(int16_t, set[i]);
        else
            ri16 = CAST_TYPE_FAIL(int16_t, set[i]);

        // should always succeed to same type
        ru16 = CAST_TYPE(uint16_t, set[i]);

        // should always succeed for larger signed types
        ri32 = CAST_TYPE(int32_t, set[i]);

        // should always succeed for larger types of same signedness
        ru32 = CAST_TYPE(uint32_t, set[i]);

        // should always succeed for larger signed types
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should always succeed for larger types of same signedness
        ru64 = CAST_TYPE(uint64_t, set[i]);
    }
}

#define INT32_VARIANTS 5
void
i32_test()
{
    printf("Checking conversion from int32_t\n");
    // all relevant variantes
    int32_t set[INT32_VARIANTS] = {INT32_MIN, -1, 0, 1, INT32_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < INT32_VARIANTS; i++) {
        printf("Checking value: %d\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i >= 2 && i <= 3)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should only succeed for [target_min,0,target_max]
        if (i >= 1 && i <= 3)
            ri8 = CAST_TYPE(int8_t, set[i]);
        else
            ri8 = CAST_TYPE_FAIL(int8_t, set[i]);

        // should succeed for [0,target_max]
        if (i >= 2 && i <= 3)
            ru8 = CAST_TYPE(uint8_t, set[i]);
        else
            ru8 = CAST_TYPE_FAIL(uint8_t, set[i]);

        // should only succeed for [target_min,0,target_max]
        if (i >= 1 && i <= 3)
            ri16 = CAST_TYPE(int16_t, set[i]);
        else
            ri16 = CAST_TYPE_FAIL(int16_t, set[i]);

        // should succeed for [0,target_max]
        if (i >= 2 && i <= 3)
            ru16 = CAST_TYPE(uint16_t, set[i]);
        else
            ru16 = CAST_TYPE_FAIL(uint16_t, set[i]);

        // should always succeed to same type
        ri32 = CAST_TYPE(int32_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru32 = CAST_TYPE(uint32_t, set[i]);
        else
            ru32 = CAST_TYPE_FAIL(uint32_t, set[i]);

        // should always succeed for larger types of same signedness
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru64 = CAST_TYPE(uint64_t, set[i]);
        else
            ru64 = CAST_TYPE_FAIL(uint64_t, set[i]);
    }
}

#define UINT32_VARIANTS 4
void
u32_test()
{
    printf("Checking conversion from uint32_t (sizeof: %lu)\n",
           sizeof(uint32_t));
    // all relevant variantes
    uint32_t set[UINT32_VARIANTS] = {0, 1, INT32_MAX, UINT32_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < UINT32_VARIANTS; i++) {
        printf("Checking value: %u\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i <= 1)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should only succeed for [0,target_max]
        if (i <= 1) {
            ri8  = CAST_TYPE(int8_t, set[i]);
            ru8  = CAST_TYPE(uint8_t, set[i]);
            ri16 = CAST_TYPE(int16_t, set[i]);
            ru16 = CAST_TYPE(uint16_t, set[i]);
        } else {
            ri8  = CAST_TYPE_FAIL(int8_t, set[i]);
            ru8  = CAST_TYPE_FAIL(uint8_t, set[i]);
            ri16 = CAST_TYPE_FAIL(int16_t, set[i]);
            ru16 = CAST_TYPE_FAIL(uint16_t, set[i]);
        }
        // should succeed up to (including) signed max
        if (i <= 2)
            ri32 = CAST_TYPE(int32_t, set[i]);
        else
            ri32 = CAST_TYPE_FAIL(int32_t, set[i]);

        // should always succeed to same type
        ru32 = CAST_TYPE(uint32_t, set[i]);

        // should always succeed for larger signed types
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should always succeed for larger types of same signedness
        ru64 = CAST_TYPE(uint64_t, set[i]);
    }
}

#define INT64_VARIANTS 5
void
i64_test()
{
    printf("Checking conversion from int64_t\n");
    // all relevant variantes
    int64_t set[INT8_VARIANTS] = {INT64_MIN, -1, 0, 1, INT64_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < INT64_VARIANTS; i++) {
        printf("Checking value: %ld\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i >= 2 && i <= 3)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should only succeed for [target_min,0,target_max]
        if (i >= 1 && i <= 3)
            ri8 = CAST_TYPE(int8_t, set[i]);
        else
            ri8 = CAST_TYPE_FAIL(int8_t, set[i]);

        // should succeed for [0,target_max]
        if (i >= 2 && i <= 3)
            ru8 = CAST_TYPE(uint8_t, set[i]);
        else
            ru8 = CAST_TYPE_FAIL(uint8_t, set[i]);

        // should only succeed for [target_min,0,target_max]
        if (i >= 1 && i <= 3)
            ri16 = CAST_TYPE(int16_t, set[i]);
        else
            ri16 = CAST_TYPE_FAIL(int16_t, set[i]);

        // should succeed for [0,target_max]
        if (i >= 2 && i <= 3)
            ru16 = CAST_TYPE(uint16_t, set[i]);
        else
            ru16 = CAST_TYPE_FAIL(uint16_t, set[i]);

        // should only succeed for [target_min,0,target_max]
        if (i >= 1 && i <= 3)
            ri32 = CAST_TYPE(int32_t, set[i]);
        else
            ri32 = CAST_TYPE_FAIL(int32_t, set[i]);

        // should succeed for [0,target_max]
        if (i >= 2 && i <= 3)
            ru32 = CAST_TYPE(uint32_t, set[i]);
        else
            ru32 = CAST_TYPE_FAIL(uint32_t, set[i]);

        // should always succeed to same type
        ri64 = CAST_TYPE(int64_t, set[i]);

        // should only succeed for 0 and positive values
        if (i >= 2)
            ru64 = CAST_TYPE(uint64_t, set[i]);
        else
            ru64 = CAST_TYPE_FAIL(uint64_t, set[i]);
    }
}

#define UINT64_VARIANTS 4
void
u64_test()
{
    printf("Checking conversion from uint64_t\n");
    // all relevant variantes
    uint64_t set[UINT64_VARIANTS] = {0, 1, INT64_MAX, UINT64_MAX};

    // go through all relevant values
    for (uint64_t i = 0; i < UINT64_VARIANTS; i++) {
        printf("Checking value: %lu\n", set[i]);

        // check conversions

        // should succeed for 0 and 1
        if (i <= 1)
            rb = CAST_TYPE(bool, set[i]);
        else
            rb = CAST_TYPE_FAIL(bool, set[i]);

        // should only succeed for [0,target_max]
        if (i <= 1) {
            ri8  = CAST_TYPE(int8_t, set[i]);
            ru8  = CAST_TYPE(uint8_t, set[i]);
            ri16 = CAST_TYPE(int16_t, set[i]);
            ru16 = CAST_TYPE(uint16_t, set[i]);
            ri32 = CAST_TYPE(int32_t, set[i]);
            ru32 = CAST_TYPE(uint32_t, set[i]);
        } else {
            ri8  = CAST_TYPE_FAIL(int8_t, set[i]);
            ru8  = CAST_TYPE_FAIL(uint8_t, set[i]);
            ri16 = CAST_TYPE_FAIL(int16_t, set[i]);
            ru16 = CAST_TYPE_FAIL(uint16_t, set[i]);
            ri32 = CAST_TYPE_FAIL(int32_t, set[i]);
            ru32 = CAST_TYPE_FAIL(uint32_t, set[i]);
        }
        // should succeed up to (including) signed max
        if (i <= 2)
            ri64 = CAST_TYPE(int64_t, set[i]);
        else
            ri64 = CAST_TYPE_FAIL(int64_t, set[i]);

        // should always succeed to same type
        ru64 = CAST_TYPE(uint64_t, set[i]);
    }
}

#if defined(__clang__)
_Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
_Pragma("GCC diagnostic pop")
#else
    #warning "Unknown compiler"
#endif

    int main()
{
    void (*tests[])() = {helper_test, bool_test, i8_test,  u8_test,  i16_test,
                         u16_test,    i32_test,  u32_test, i64_test, u64_test};
    for (size_t i = 0; i < sizeof tests / sizeof(void (*)(void)); i++) {
        tests[i]();
    }
    return 0;
}
