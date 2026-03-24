// clang-format off
// RUN: rm -f %s.replay.trace
// RUN: %lotto %stress -r 1 -o "" -- %b
// RUN: ! test -f %s.replay.trace
// clang-format on

#include <time.h>

int
main()
{
    time(NULL);
    return 0;
}

