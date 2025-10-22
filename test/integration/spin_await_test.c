// RUN: %lotto stress -r 1 -- %b | %check
// CHECK: [lotto-mock] subcommand=stress
// CHECK: [lotto-mock] target=spin_await_test
// CHECK: [lotto-mock] status=ok

#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>

#include <lotto/await.h>

static const unsigned kMaxIters = 8;

static atomic_uint g_a = 0;
static atomic_uint g_b = 0;
static atomic_uint g_c = 0;
static atomic_uint g_d = 0;

static void *
run_a(void *arg)
{
    (void)arg;
    for (unsigned i = 0; i < kMaxIters; ++i) {
        while (atomic_load_explicit(&g_d, memory_order_acquire) != i) {
            lotto_await(&g_d);
        }
        atomic_fetch_add_explicit(&g_a, 1U, memory_order_release);
    }
    return NULL;
}

static void *
run_b(void *arg)
{
    (void)arg;
    for (unsigned i = 0; i < kMaxIters; ++i) {
        while (atomic_load_explicit(&g_a, memory_order_acquire) != i + 1U) {
            lotto_await(&g_a);
        }
        atomic_fetch_add_explicit(&g_b, 1U, memory_order_release);
    }
    return NULL;
}

static void *
run_c(void *arg)
{
    (void)arg;
    for (unsigned i = 0; i < kMaxIters; ++i) {
        while (atomic_load_explicit(&g_b, memory_order_acquire) != i + 1U) {
            lotto_await(&g_b);
        }
        atomic_fetch_add_explicit(&g_c, 1U, memory_order_release);
    }
    return NULL;
}

static void *
run_d(void *arg)
{
    (void)arg;
    for (unsigned i = 0; i < kMaxIters; ++i) {
        while (atomic_load_explicit(&g_c, memory_order_acquire) != i + 1U) {
            lotto_await(&g_c);
        }
        atomic_fetch_add_explicit(&g_d, 1U, memory_order_release);
    }
    return NULL;
}

int
main(void)
{
    pthread_t a;
    pthread_t b;
    pthread_t c;
    pthread_t d;

    pthread_create(&a, NULL, run_a, NULL);
    pthread_create(&b, NULL, run_b, NULL);
    pthread_create(&c, NULL, run_c, NULL);
    pthread_create(&d, NULL, run_d, NULL);

    pthread_join(a, NULL);
    pthread_join(b, NULL);
    pthread_join(c, NULL);
    pthread_join(d, NULL);

    assert(atomic_load_explicit(&g_a, memory_order_relaxed) == kMaxIters);
    assert(atomic_load_explicit(&g_b, memory_order_relaxed) == kMaxIters);
    assert(atomic_load_explicit(&g_c, memory_order_relaxed) == kMaxIters);
    assert(atomic_load_explicit(&g_d, memory_order_relaxed) == kMaxIters);

    return 0;
}
