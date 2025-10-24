// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: %lotto stress -s random -r 5 -- %b
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#include <lotto/await_while.h>

_Atomic unsigned x = 0, y = 0, z = 0, a = 0, b = 0, finished_threads = 0;
_Atomic unsigned nested1 = 0, nested2 = 0;
const unsigned NUM_THREADS = 5;


void *
run_1()
{
    x = 0;
    await_while (x <= 3) {
        x += 1;
    }
    finished_threads += 1;
    return NULL;
}
void *
run_2()
{
    y = 0;
    await_while (y == 0) {
        y = z;
        z = 1;
    }
    finished_threads += 1;
    return NULL;
}

/**
 * @brief spin_loop with writes using a local variable
 * Actually this fails, because the compiler might optimize this
 * to use registers. This is no real problem but is something
 * to be aware of.
 */
void *
run_3()
{
    int local = 0;
    await_while (local <= 3) {
        local += 1;
    }
    finished_threads += 1;
    return NULL;
}

void *
run_alice()
{
    a = 0;
    await_while (b == 0) {
        a = 1;
    }
    finished_threads += 1;
    return NULL;
}
void *
run_bob()
{
    a = 0;
    await_while (a == 0)
        ;
    b = 1;
    finished_threads += 1;
    return NULL;
}

void *
run_nested()
{
    await_while (nested2 <= 4) {
        nested1 = 0;
        await_while (nested1 <= 1) {
            nested2++;
            nested1++;
        }
    }
    finished_threads += 1;
    return NULL;
}

int
main()
{
    pthread_t t1;
    pthread_t t2;
    pthread_t alice;
    pthread_t bob;
    pthread_t nested;
    pthread_create(&t1, 0, run_1, NULL);
    pthread_create(&t2, 0, run_2, NULL);
    pthread_create(&alice, 0, run_alice, NULL);
    pthread_create(&bob, 0, run_bob, NULL);
    pthread_create(&nested, 0, run_nested, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(alice, NULL);
    pthread_join(bob, NULL);
    pthread_join(nested, NULL);

    assert(finished_threads == NUM_THREADS);
    return 0;
}
