# Lotto

A tool to **debug** concurrent systems. Lotto helps the developer to find, reproduce and understand concurrency bugs.
Note that Lotto's primiry target **is not** searching for unknown bugs, but rather debugging known bugs.

Lotto intercepts and controls the execution of multithreaded systems, exploring schedulings that are likely to trigger concurrent bugs.
The developer can tune Lotto's bug search by [adding annotations](doc/api/lotto/README.md) to the system under test.
Once the target bug is triggered, Lotto is capable of replaying it with or without `gdb`.
Finally, to help the develper to understand the bug, Lotto can try to find crucial scheduling decisions that contribute to the bug being triggered (see [inflection point](doc/inflex.md)).

Lotto can be built in three flavors:

- **pLotto**: POSIX Lotto links to a target application and controls the scheduling of the application threads.
- **qLotto**: QEMU-based Lotto runs an arm64 VM and controls the scheduling of the virtual cores.

## Quick start

Refer to the [documentation](doc/install.md) for instructions on how to build and install Lotto.
Here we have a short explanation how to compile and run *pLotto*.
Assuming a Ubuntu/Debian, get the minimal dependencies:

	sudo apt install cmake gcc xxd


Now create a build directory and compile the default Lotto configuration

    cmake -S. -Bbuild
    cd build
    make all

With `make test` you can run the unit tests, but also try running Lotto with a real program.
For that run the following command inside the `build` directory:

    ./lotto stress ./test/integration/no_mutual_exclusion

After a few seconds you should see something like the following:

```
[lotto] round: 0/inf, pos
[lotto] round: 1/inf, pos
[lotto] round: 2/inf, pos
[lotto] round: 3/inf, pos
[lotto] round: 4/inf, pos
[lotto] round: 5/inf, pos
[lotto] round: 6/inf, pos
[lotto] round: 7/inf, pos
[lotto] round: 8/inf, pos
[lotto] round: 9/inf, pos
[lotto] round: 10/inf, pos
[2] assert failed run(): ../test/integration/no_mutual_exclusion.c:16: x == 1

/tmp/liblotto/libplotto.so(+0x1805a)[0x7ffff7f7b05a]
/tmp/liblotto/libplotto.so(__assert_fail+0x101)[0x7ffff7f7b480]
./test/integration/no_mutual_exclusion(run+0xc1)[0x55555555538a]
/tmp/liblotto/libplotto.so(+0xf998)[0x7ffff7f72998]
/lib/x86_64-linux-gnu/libc.so.6(+0x94ac3)[0x7ffff7c6cac3]
/lib/x86_64-linux-gnu/libc.so.6(+0x126a40)[0x7ffff7cfea40]
```

In this example, Lotto ran the program 10 times to be able to trigger the assertion.
With `./lotto replay` you can replay the failed execution as often as you like.
If you launch the replay inside `gdb` you must set `follow-fork-mode` to `child`.
Use `./lotto debug` to start the replay in `gdb` and have it automatically configured.

> Note that *pLotto* relies on TSAN instrumentation to capture execution events.
> **All** files in your program **must** be compiled and linked with the `-fsanitize=thread` option.
> Currently only `gcc` is supported, compiling with `clang` is known to cause problems.

## Next steps

See the [installation guide](doc/install.md) for more compilation options for *pLotto*.



