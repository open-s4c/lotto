# FAQ


## Different toolchains

### How to know if my gcc/clang supports TSAN instrumentation

Creare a test file

    cat > test.c <<EOF
    int x;
    int main() { x = 1; return 0; }
    EOF

Compile and check if it requires TSAN symbols:

    clang -fsanitize=thread -c test.c
    nm test.o

The output of nm should be something like this:

    0000000000000000 T main
                     U __tsan_func_entry
                     U __tsan_func_exit
                     U __tsan_init
    0000000000000000 t tsan.module_ctor
                     U __tsan_write4
    0000000000000000 B x


If the clang or gcc of your toolchain produces an object with "U" marked `__tsan` symbols, we can run it with plotto.


Note that `libtsan` is **not** required and compiling test.c to an executable may fail due to missint `libtsan`.

You can use our `libtsano` instead:

cd /path/to/tsano
mv libtsano.so libtsan.so.0     ## mind the "tsan" !

cd /path/to/test.c
LD_LIBRARY_PATH=/path/to/tsano clang -o test.bin test.o -ltsan

This should produce a valid executable.

