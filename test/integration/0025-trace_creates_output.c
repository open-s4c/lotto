// clang-format off
// RUN: rm -f %s.replay.trace
// RUN: %lotto trace %{tempdir} -o %s.replay.trace -- %b
// RUN: test -s %s.replay.trace
// clang-format on

int
main(void)
{
    return 0;
}
