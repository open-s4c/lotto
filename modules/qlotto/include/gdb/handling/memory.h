#ifndef LOTTO_GDB_MEMORY_H
#define LOTTO_GDB_MEMORY_H

#include <stdint.h>

int64_t gdb_srv_handle_m(int fd, uint8_t *msg, uint64_t msg_len);

#endif // LOTTO_GDB_MEMORY_H
