# Lotto-perf

Lotto perf is a handler that will inform the user about the amount of
captured event's of each type, and the number of times each line of 
code called an event.

## Why not use `perf record`

When using `perf record`, most functions will have around 0.21% contribution or less,
because most of the runtime is not spent on the actual user code, but on lotto.

`lotto-perf` allows the user to get information on how to improve the code, ignoring
lotto's machinery runtime.

## How to use it

First, compile it with the following flags and enviroment variables:

```bash
cmake -S. -Bbuild -DLOTTO_RUST=ON # enable rust
cmake --build build
export LOTTO_PERF=true # must set to true
export RUST_LOG=debug # not needed, but could also be useful
```

After running, a file `lotto-perf.json` will be generated on the current directory.

### Pretty printing

To pretty print the `json` use the script at `./lotto/scripts/lotto-perf-pretty-print.py`.

You can run it with no arguments, and it will look for the file in `./lotto/build/` or feed the JSON path:

```bash
# implicit. Will look in build/lotto-perf.json
python3 lotto-perf-pretty-print.py
# explicit path
python3 lotto-perf-pretty-print.py other-path/name.json 
```
