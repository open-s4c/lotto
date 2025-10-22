// clang-format off
// UNSUPPORTED: aarch64, clang
// RUN: (! %lotto %stress --stable-address-method MASK -- %b 2>&1) | %check --check-prefix=BUG
// RUN: %lotto %inflex -r 30 --inflex-method=be -- %b >/dev/null 2>&1
// RUN: printf '\nrun-replay-lotto\n' | %lotto %debug -- %b | %check --check-prefix=LOC
// BUG: assert failed {{.*}}/inflex_method_{{.*}}.c:{{[0-9]+}}: x != 0b11
// LOC: x++;
// clang-format on

#include <pthread.h>

static void *
noop(void *arg)
{
    (void)arg;
    return NULL;
}

int
main(void)
{
    pthread_t thread1;
    pthread_create(&thread1, NULL, noop, NULL);
    pthread_join(thread1, NULL);
    return 0;
}
