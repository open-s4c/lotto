// clang-format off
// UNSUPPORTED: aarch64
// RUN: (! %lotto %stress --stable-address-method MASK -- %b 2>&1) | %check --check-prefix=BUG
// RUN: env LOTTO_MOCK_TEST=inflex_abc_test %lotto %inflex -r 10 &>/dev/null
// RUN: env LOTTO_MOCK_TEST=inflex_abc_test %lotto %debug --file-filter="libvsync" <<< $'\n'run-replay-lotto | %check --check-prefix=LOC
// BUG: assert failed {{.*}}/inflex_abc.c:{{[0-9]+}}: recv_from[i] == true
// LOC: uint32_t id = vatomic32_read(&next_id);
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
    pthread_t producers[2];
    pthread_t consumer;

    for (size_t i = 0; i < 2; ++i) {
        pthread_create(&producers[i], NULL, noop, NULL);
    }
    pthread_create(&consumer, NULL, noop, NULL);

    for (size_t i = 0; i < 2; ++i) {
        pthread_join(producers[i], NULL);
    }
    pthread_join(consumer, NULL);
    return 0;
}
