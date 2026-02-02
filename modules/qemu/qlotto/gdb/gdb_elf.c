/*
 */

#include <fcntl.h>
#include <gelf.h>
#include <libelf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include <lotto/qlotto/gdb/gdb_readelf.h>
#include <lotto/unsafe/_sys.h>

elf_symbol_t *symbols_head = NULL;

void
qlotto_symbol_add(uint64_t addr, uint64_t size, char *func_name)
{
    elf_symbol_t *cur = symbols_head;
    while (cur->next != NULL) {
        if (cur->next->addr == addr) { // symbol entry exists already
            // logger_infof( "Symbol already exists 0x%lx\n", addr);
            return;
        }
        cur = cur->next;
    }
    // symbol list traversed, add at the end
    cur->next      = (elf_symbol_t *)rl_malloc(sizeof(elf_symbol_t));
    cur            = cur->next;
    cur->next      = NULL;
    cur->addr      = addr;
    cur->size      = size;
    cur->func_name = (char *)rl_malloc(rl_strlen(func_name) + 1);
    rl_memcpy(cur->func_name, func_name, rl_strlen(func_name));
    cur->func_name[rl_strlen(func_name)] = '\0';
    // logger_infof( "Symbol added 0x%lx %s\n", addr, func_name);
}

void
qlotto_symbol_del(uint64_t addr)
{
    elf_symbol_t *cur  = symbols_head;
    elf_symbol_t *last = NULL;
    while (cur->next != NULL && cur->next->addr != addr) {
        last = cur;
        cur  = cur->next;
    }

    // symbol found or list traversed -> check which

    if (cur->next == NULL) // not found
        return;

    last = cur;
    cur  = cur->next;

    if (cur->addr == addr) { // breakpoint found, delete it
        ASSERT(last != NULL);
        last->next = cur->next;
        cur->next  = NULL;
        cur->addr  = 0;
        cur->size  = 0;
        rl_free(cur->func_name);
        cur->func_name = NULL;
        rl_free(cur);
        // logger_infof( "BP deleted 0x%lx\n", b_pc);
    }
}

elf_symbol_t *
qlotto_symbol_get_by_name(const char *func_name)
{
    elf_symbol_t *cur = symbols_head;
    while (cur->next != NULL) {
        if (strcmp(cur->next->func_name, func_name) == 0) // found symbol
            return cur->next;
        cur = cur->next;
    }
    return NULL;
}

elf_symbol_t *
qlotto_symbol_get_by_addr(uint64_t addr)
{
    elf_symbol_t *cur = symbols_head;
    while (cur->next != NULL) {
        if (cur->next->addr <= addr &&
            cur->next->addr + cur->next->size > addr) // found symbol
            return cur->next;
        cur = cur->next;
    }
    return NULL;
}

void
gdb_get_symbols(const char *filename)
{
    logger_infof("Got string: %s\n", filename);
    int fd  = 0;
    Elf *e  = NULL;
    char *k = NULL;
    Elf_Kind ek;
    Elf_Scn *scn = NULL;
    GElf_Shdr shdr;
    Elf_Data *data = NULL;
    GElf_Sym sym;
    uint64_t num = 0;
    uint64_t idx = 0;

    if (elf_version(EV_CURRENT) == EV_NONE) {
        logger_errorf(" ELF library initialization failed : %s ", elf_errmsg(-1));
    }

    if ((fd = rl_open(filename, O_RDONLY, 0)) < 0) {
        logger_errorf("open \"%s\" failed\n", filename);
    }

    if ((e = elf_begin(fd, ELF_C_READ, NULL)) == NULL) {
        logger_errorf(" elf_begin() failed : %s.\n", elf_errmsg(-1));
    }

    ek = elf_kind(e);
    switch (ek) {
        case ELF_K_AR:
            k = "ar(1) archive";
            break;
        case ELF_K_ELF:
            k = "elf object";
            break;
        case ELF_K_NONE:
            k = "data";
            break;
        default:
            k = "unrecognized";
    }

    if (ek != ELF_K_ELF) {
        logger_errorf("%s of type %s\n", filename, k);
        logger_errorf("Only specify ELF files for symbol lookup.\n");
    }

    // got elf file, look for symbol table
    while ((scn = elf_nextscn(e, scn)) != NULL) {
        gelf_getshdr(scn, &shdr);
        if (SHT_SYMTAB == shdr.sh_type) {
            /* stop and iterate over symbol table */
            break;
        }
    }

    // got symbol table, traverse it.
    data = elf_getdata(scn, NULL);
    num  = shdr.sh_size / shdr.sh_entsize;

    bool skip = true;

    while (idx < num) {
        skip = true;
        gelf_getsym(data, (int32_t)idx, &sym);

        if (sym.st_info == 2)
            skip = false;

        if (sym.st_other == 0)
            skip = false;

        if (skip) {
            idx++;
            continue;
        }

        qlotto_symbol_add(sym.st_value, sym.st_size,
                          elf_strptr(e, shdr.sh_link, sym.st_name));

        idx++;
    }

    elf_end(e);
    rl_close(fd);

    logger_infof("Done with %s\n", filename);
}

void
gdb_read_symbols_env(void)
{
    char path[2048];
    const char *elfs     = getenv("LOTTO_ELF_UNSTRIPPED");
    uint64_t elfs_idx    = 0;
    uint64_t path_len    = 0;
    const char *path_cur = NULL;
    const char *path_sep = NULL;
    char *next_sep       = NULL;

    do {
        path_cur = elfs + elfs_idx;
        path_sep = next_sep = strchr(elfs + elfs_idx, ':');

        if (path_sep == NULL)
            path_sep = elfs + rl_strlen(elfs);

        path_len = path_sep - (elfs + elfs_idx);

        if (path_len == 0) {
            elfs_idx++;
            continue;
        }

        rl_memcpy(path, path_cur, path_len);
        path[path_len] = '\0';

        // do something with substring

        gdb_get_symbols(path);

        elfs_idx += path_len + 1;
    } while (next_sep);
}

void
gdb_read_symbols(void)
{
    symbols_head            = (elf_symbol_t *)rl_malloc(sizeof(elf_symbol_t));
    symbols_head->next      = NULL;
    symbols_head->addr      = 0;
    symbols_head->size      = 0;
    symbols_head->func_name = NULL;

    gdb_read_symbols_env();
}
