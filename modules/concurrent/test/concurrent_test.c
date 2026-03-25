// clang-format off
// RUN: %lotto %record -- %b | grep CONCURRENT_LOG > %b.record
// RUN: %lotto %replay | grep CONCURRENT_LOG > %b.replay
// RUN: diff %b.record %b.replay
// clang-format on

#include <assert.h>
#include <sched.h>
#include <stdio.h>

#include <lotto/unsafe/concurrent.h>

int
main(void)
{
    setbuf(stdout, NULL);

    printf("CONCURRENT_LOG before %d\n", lotto_is_concurrent());
    assert(!lotto_is_concurrent());

    lotto_concurrent({
        printf("CONCURRENT_LOG inside-a %d\n", lotto_is_concurrent());
        assert(lotto_is_concurrent());
        sched_yield();
        printf("CONCURRENT_LOG inside-b %d\n", lotto_is_concurrent());
        assert(lotto_is_concurrent());
    });

    printf("CONCURRENT_LOG after %d\n", lotto_is_concurrent());
    assert(!lotto_is_concurrent());
    return 0;
}
