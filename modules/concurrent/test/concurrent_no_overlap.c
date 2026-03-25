// clang-format off
// RUN: %lotto %record -- %b | grep CONCURRENT_LOG > %b.record
// RUN: %lotto %replay | grep CONCURRENT_LOG > %b.replay
// RUN: diff %b.record %b.replay
// clang-format on

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#include <vsync/atomic.h>

static volatile int owner_;
static volatile int overlap_;
static vatomic32_t start_ = VATOMIC_INIT(0);
static vatomic32_t ready_ = VATOMIC_INIT(0);

#if defined(__clang__) || defined(__GNUC__)
__attribute__((no_sanitize("thread")))
#endif
static void
critical_body_(int tid)
{
    if (owner_ != 0) {
        overlap_ = 1;
    }
    owner_ = tid;

    for (volatile uint64_t i = 0; i < 20000000ULL; ++i) {
        if (owner_ != tid) {
            overlap_ = 1;
        }
    }

    owner_ = 0;
}

static void *
thread_(void *arg)
{
    int tid = (int)(uintptr_t)arg;

    vatomic_inc(&ready_);
    while (!vatomic_read(&start_))
        ;

    critical_body_(tid);
    return NULL;
}

int
main(void)
{
    pthread_t t1;
    pthread_t t2;

    setbuf(stdout, NULL);

    assert(pthread_create(&t1, NULL, thread_, (void *)(uintptr_t)1) == 0);
    assert(pthread_create(&t2, NULL, thread_, (void *)(uintptr_t)2) == 0);
    while (vatomic_read(&ready_) != 2)
        ;
    vatomic_write(&start_, 1);
    assert(pthread_join(t1, NULL) == 0);
    assert(pthread_join(t2, NULL) == 0);

    printf("CONCURRENT_LOG overlap %d\n", overlap_);
    assert(overlap_ == 0);
    return 0;
}
