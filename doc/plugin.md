# Plugins

## Name conventions

Plugin filenames must begin with `liblotto_`, and end with either `engine.so`
(engine plugin) or `cli.so` (CLI plugin).

- CLI plugins are loaded by `lotto`, the CLI program.
- Engine plugins are loaded by `libplotto.so` in the child processes.

## Plugin loading

Lotto will try in order:

1. The directory `CMAKE_BINARY_DIR/plugins`, determined during building
2. The directory `CMAKE_INSTALL_PREFIX/share/lotto/plugins`, determined during
   building
3. The directory specified by the plugin argument --plugin-dir DIR before any
   Lotto command
