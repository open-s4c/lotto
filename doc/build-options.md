# Lotto build options

- `LOTTO_BUSYLOOP_FUTEX`: (default `OFF`)

  Replace futex with a busyloop in switcher

- `LOTTO_EMBED_LIB`: (default `ON`)

  Embed `liblotto-runtime.so` in the Lotto CLI binary

- `LOTTO_EXECINFO`: (default `ON`)

  Use execinfo in Lotto runtime

- `LOTTO_FRONTEND`: (default `POSIX`)

  Lotto frontend

- `LOTTO_RACKET_TESTS`: (default `OFF`)

  Enable Racket tests

- `LOTTO_RUNTIME_MEMMGR`: (default `LIBC`)

  Lotto runtime memory manager. Options: LIBC;LEAK;UAF;MEMPOOL

- `LOTTO_RUNTIME_MEMPOOL_NPAGES`: (default `1000`)

  Number of pages in Lotto runtime mempool when LOTTO_RUNTIME_MEMMGR=MEMPOOL.

- `LOTTO_MODULE_rusty`: (default `ON` if `cargo` is found, `OFF` otherwise)

  Enable Lotto Rust module

- `LOTTO_STABLE_ADDRESS_MAP`: (default `ON`)

  whether stable addresses support the `MAP` method

- `LOTTO_TESTS_WITH_TSAN`: (default `ON`)

  Enable TSAN instrumentation in tests

- `LOTTO_TEST_COVERAGE`: (default `OFF`)

  Enable test coverage with lcov

- `LOTTO_TSAN_RUNTIME`: (default `ON`)

  Use TSAN in Lotto runtime

- `LOTTO_USER_MEMMGR`: (default `LIBC`)

  Lotto user memory manager. Options: LIBC;LEAK;UAF;MEMPOOL

- `LOTTO_USER_MEMPOOL_NPAGES`: (default `1000`)

  Number of pages in Lotto user mempool when LOTTO_USER_MEMMGR=MEMPOOL.


# Further options

## Libvsync options

- `LIBVSYNC_ADDRESS_SANITIZER`: (default `OFF`)

  Compile with -fsanitize=address

- `LIBVSYNC_BUILD_TESTS`: (default `OFF`)

  Build the tests for libvsync

- `LIBVSYNC_CROSS_COMPILE`: (default `OFF`)

  Cross-compile libvsync with selected toolchains

- `LIBVSYNC_CROSS_COMPILE_DIR`: (default `atomics`)

  which directory should be built and tested (make ah,-C DIR test)

- `LIBVSYNC_CROSS_COMPILE_TOOLCHAINS`: (default `x86_64;armel;armel8;armeb;armeb8;arm64;arm64_lse`)

  semicolon-separated list of toolchains

- `LIBVSYNC_DBG_ENABLE`: (default `OFF`)

  Enables debug prints in code and tests


## Capstone options



## Lit options

- `LOTTO_LIT_DAILY_TIMEOUT`: (default `180`)

  Lit daily timeout

- `LOTTO_LIT_NIGHTLY_TIMEOUT`: (default `600`)

  Lit nightly timeout

- `LOTTO_LIT_TIMEOUT`: (default `300`)

  Lit timeout

- `LOTTO_LIT_WORKERS`: (default `1`)

  Number of lit workers
