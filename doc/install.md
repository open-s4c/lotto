# Building and Installing Lotto

## Build and test dependencies

- cmake
- gcc
- xxd
- gdb-tools yapf lit filecheck psutil

In Ubuntu/Debian, run:

    sudo apt install cmake gcc xxd
    pip install gdb-tools yapf lit filecheck psutil

## Build and install Lotto

Clone the lotto repository and switch to that directory. You can then build and install lotto with CMake like this:

```bash
cmake -S. -Bbuild
cmake --build build
cmake --install build # optionally add `--prefix $HOME/.local` to install to a local directory 
```

Be sure to add the installation location (e.g. `$HOME/.local/bin`) to your `PATH` environment variable.
In principle the `lotto` executable is self-contained and can also be copied directly from the `build` directory.


### Cross-compiling lotto

Lotto provides a selected number of CMake toolchain files, to easily cross-compile to certain targets. 
Currently, this includes:

- `cmake/aarch64-linux-ohos.cmake`

Each of these toolchain files includes additional documentation at the top of that file.

To cross-compile to one of our supported targets configure cmake with the `--toolchain` parameter before building.
Some toolchain files may require setting some additional environment variables beforehand, for example
`OHOS_SDK_ROOT_DIR` must be set to the location of the OHOS SDK when compiling lotto for OpenHarmony.

Example: 
```bash
export OHOS_SDK_ROOT_DIR=/path/to/trunk_1130
cmake -S. -Bbuild --toolchain cmake/aarch64-linux-ohos.cmake
cmake --build build
```

### Build Options

Currently, lotto only supports being built in `Debug` mode.

- `LOTTO_RUST`: A subset of (optional) lotto handlers is implemented in Rust. Set to `ON` to enable the handlers.
                See [rust.md](rust.md) for detailed instructions on installing the Rust compiler.


### Advanced compatibility CMake options

If your target architecture is already supported by lotto with an official toolchain file 
(see the [Cross-compiling-lotto](#cross-compiling-lotto) section) you may skip this section, since all the required
variables would already be set by the toolchain file.

The following advanced cache variables can be set to tweak the behavior of lotto for pecularities of some systems.

- `LOTTO_EXECINFO`: Set this variable to `OFF` if your target system does not provide `execinfo.h`
- `LOTTO_TSAN_RUNTIME`: Set this variable to `OFF` if lotto should not intercept tsan calls.
- `LOTTO_TESTS_WITH_TSAN`: Set this variable to `OFF` if lotto tests should not be instrumented with TSAN.

## Compile target program

User space Lotto (aka p-lotto) relies on TSAN instrumentation to capture execution events.
**All** files in your program **must** be compiled and linked with the `-fsanitize=thread` option.
Currently only `gcc` is supported, compiling with `clang` is known to cause problems.
