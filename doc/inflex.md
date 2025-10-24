# Inflection point search

The inflection point is the earliest point at the execution trace after which all schedules result in a failure. Lotto can find the inflection point of a failed execution in the following way:

    lotto stress -s pos test/integration/inflex_abc # should find a bug and save the trace in replay.trace
    lotto inflex -r 50 # trims replay.trace to its inflection point

It is important to select a reasonably small but sufficient -r (usually twice as much as the number of stress runs to find the bug is enough) for inflex because it is MAX_INT by default. Lotto can show the inflection point in the code with its debug command:

    lotto debug # launches gdb and sets breakpoints
    run-replay-lotto # runs the program until the end of the trace (inflection point)

### How to figure out which libraries I should instrument

If your build process is complicated, and you are not sure what code is linked into your binary, you could
use `llvm-readelf -d | grep NEEDED` to get the list of dynamic libraries directly linked into your code,
locate.

[Go back to main](../README.md)
