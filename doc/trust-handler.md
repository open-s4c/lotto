# Usage

To run it first compile lotto with the rust engine on:
```sh
cmake -S. -Bbuild -DLOTTO_RUST=ON
make -C build -j
```

Now run lotto stress with the flag `--handler-trust` enabled. In addition, you can also enable the data-race handler:

```sh
lotto stress file_bin --handler-trust enable --handler-race-strict enable
```

The program will run until all the execution graphs are visited, in which case it will also print the number of such graphs, or until an error is found.

# References

[TruST paper](https://plv.mpi-sws.org/genmc/popl2022-trust-full.pdf)

There is also a lot of documentation in the code: [trust code](/src/engine/rust/trust/src/lib.rs), [handler code](/src/engine/rust/handlers/trust/src/lib.rs).


# Issues and next steps

Here we high-light some of the issues, and potential next steps, for the handler.

- **Handling the creation of new threads**:
    As in the paper, we consider that all the threads are created in the beginning of the program by some other thread that does not interact with any address. For example, all threads are created in main at the same time, and this thread does not have other events prior to that.

    We need to both remove this assumption and also add dependencies between the events `thread_create` respectively `thread_join`. One idea is to treat these events as writes and reads, as well as the categories `CAT_TASK_CREATE` respectively `TASK_FINI`. For example, `thread_create` is a write from which `CAT_TASK_CREATE` reads.
- **Mixed-size accesses**:
    This is currently unsupported by the handler. To add support for mixed sized accesses, we must change the code in the iterators, and also add edges if the addresses intersect, instead of being equal.

    We must also properly set the initial values, because if we first have a 1 byte read and later a 16 bytes read, we must have the initial value of all the bytes. This can be done by loading the whole cacheline whenever it is first accessed and storing it byte-by-byte.
- **Optimize consistency checks**:
    To check if we can add a new read or write, the handler tries to insert it in the graph and then calls a `is_consistent` function, which is $O(N)$.

    It is possible to make this faster by using vector clocks (or forest clocks), in which case additions would be $O(T)$ and so would be the checks. This would also help by dropping quickly a lot of inconsistent graphs, due to violation of vector clocks' hb.

    Here $T$ is the number of threads and $N$ the number of events in total.
- **Properly dealing with Spinlocks**:
    Trying to use the `await_while` structure for spin-loops in this handler is not enough yet, because the handler is not aware of the order imposed due to one thread unblocking the other. On the TruSt paper, they decide to replace spinloops with `assume` statements. This is one of our options.
- **Refactor: Use Petgraph**:
    We are using a custom implementation of graphs in the struct [ExecutionGraph](/src/engine/rust/graph_util/execution_graph/src/lib.rs). Instead, we could use the rust library [Petgraph](https://docs.rs/petgraph/latest/petgraph/).
