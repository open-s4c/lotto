#include <stdatomic.h>

#include <lotto/log.h>

#ifndef SIZE
    #define SIZE 3
#endif

typedef struct {
    volatile int value;
    atomic_int next;
} SafeStackItem;

atomic_int head;
atomic_int count;
SafeStackItem array[SIZE];

void
init()
{
    atomic_init(&count, SIZE);
    atomic_init(&head, 0);
    for (int i = 0; i < SIZE - 1; i++)
        atomic_init(&array[i].next, i + 1);
    atomic_init(&array[SIZE - 1].next, -1);
}

int
pop()
{
    lotto_log(1);
    while (atomic_load_explicit(&count, memory_order_acquire) > 1) {
        lotto_log(2);
        lotto_log(3);
        int head1 = atomic_load_explicit(&head, memory_order_acquire);
        lotto_log_data(4, head1);
        lotto_log(5);
        int next1 = atomic_exchange_explicit(&array[head1].next, -1,
                                             memory_order_seq_cst);
        lotto_log_data(6, next1);

        if (next1 >= 0) {
            lotto_log(7);
            int head2 = head1;
            if (atomic_compare_exchange_strong_explicit(&head, &head2, next1,
                                                        memory_order_seq_cst,
                                                        memory_order_seq_cst)) {
                lotto_log(8);
                lotto_log(9);
                atomic_fetch_sub_explicit(&count, 1, memory_order_seq_cst);
                lotto_log(10);
                return head1;
            } else {
                lotto_log_data(11, head2);
                lotto_log(12);
                // Why Xchg instead of a simple store?
                atomic_exchange_explicit(&array[head1].next, next1,
                                         memory_order_seq_cst);
                lotto_log(13);
            }
        }
    }
    return -1;
}

void
push(int index)
{
    lotto_log(14);
    int head1 = atomic_load_explicit(&head, memory_order_acquire);
    do {
        lotto_log_data(15, head1);
        lotto_log(16);
        atomic_store_explicit(&array[index].next, head1, memory_order_release);
        lotto_log(17);
        lotto_log(18);
    } while (atomic_compare_exchange_strong_explicit(
                 &head, &head1, index, memory_order_seq_cst,
                 memory_order_seq_cst) == 0);
    lotto_log_data(19, head1);
    lotto_log(20);
    atomic_fetch_add_explicit(&count, 1, memory_order_seq_cst);
    lotto_log(21);
}
