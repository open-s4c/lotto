/**
 * @file gdb_server.h
 * @brief QLotto GDB declarations for gdb server.
 */
#ifndef LOTTO_QLOTTO_GDB_GDB_SERVER_H
#define LOTTO_QLOTTO_GDB_GDB_SERVER_H

#include <stdint.h>

#include <lotto/qlotto/gdb/rsp_util.h>

#define GDB_SERVER_THREAD_MAGIC 0xeebb2e6a6af19ff7

typedef struct gdb_server_thread_s {
    uint64_t magic;
} gdb_server_thread_t;

int gdb_server_start(void);

#endif // LOTTO_QLOTTO_GDB_GDB_SERVER_H
