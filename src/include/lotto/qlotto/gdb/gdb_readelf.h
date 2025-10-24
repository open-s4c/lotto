/*
 */
#ifndef LOTTO_QEMU_GDB_READELF_H
#define LOTTO_QEMU_GDB_READELF_H

typedef struct elf_symbol_s {
    struct elf_symbol_s *next;
    uint64_t addr;
    uint64_t size;
    char *func_name;
} elf_symbol_t;

void gdb_read_symbols(void);
elf_symbol_t *qlotto_symbol_get_by_name(const char *func_name);
elf_symbol_t *qlotto_symbol_get_by_addr(uint64_t addr);

#endif // LOTTO_QEMU_GDB_READELF_H
