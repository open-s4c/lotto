// clang-format off
// REQUIRES: RUST_HANDLERS_AVAILABLE
// RUN: %lotto stress --handler-await-address enable --handler-spin-loop enable -s random -r 5 -- %b
// clang-format on
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>

#include <lotto/await_while.h>

const unsigned MAX_ITERS           = 10U;
const unsigned NUM_PRODUCERS       = 3U;
const unsigned expected_messages   = 30U; // MAX_ITER * NUM_PRODUCERS
_Atomic unsigned finished_threads  = 0;
_Atomic unsigned read_messages     = 0;
_Atomic unsigned received_messages = 0;

_Atomic int msg_ready[500];

int
get_msg_id()
{
    return received_messages++;
}

void *
run_producer(void *arg)
{
    // int id = *(int *)arg;
    // printf("Starting thread %d\n", id);
    for (unsigned i = 0; i < MAX_ITERS; ++i) {
        int cur = get_msg_id();
        // create the message
        // ...
        // printf("Ready: %d\n", cur);
        msg_ready[cur] = 1;
    }
    finished_threads += 1;
    // printf("Finished to run thread %d\n", id);
    return NULL;
}

void *
run_consumer(void *arg)
{
    await_while (read_messages < expected_messages) {
        await_while (read_messages < received_messages) {
            // ^ for real cases this would be a simple while
            await_while (!msg_ready[read_messages])
                ;
            // consume...
            read_messages += 1;
            // printf("\n\n Consumed message %d, received = %d, expected =
            // %d\n\n",
            //        read_messages - 1, received_messages, expected_messages);
        }
    }
    finished_threads += 1;
    return NULL;
}

int
main()
{
    for (unsigned i = 0; i < expected_messages; i++) {
        msg_ready[i] = 0;
    }
    pthread_t thr[NUM_PRODUCERS];
    int ids[NUM_PRODUCERS];
    pthread_t consumer;
    pthread_create(&consumer, 0, run_consumer, NULL);
    for (unsigned i = 0; i < NUM_PRODUCERS; i++) {
        ids[i] = i;
        pthread_create(&thr[i], 0, run_producer, (void *)&ids[i]);
    }
    for (unsigned i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(thr[i], 0);
    pthread_join(consumer, 0);
    assert(finished_threads == NUM_PRODUCERS + 1);
    assert(read_messages == expected_messages);
    for (unsigned i = 0; i < expected_messages; i++) {
        assert(msg_ready[i]);
    }
    return 0;
}
