// clang-format off
// RUN: !( timeout -k 5s 4s %ntlotto %stress -r 1 -- %b)
// clang-format on

#include <lotto/order.h>

int
main()
{
    lotto_order(2);
    return 0;
}

