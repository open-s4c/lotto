#ifndef LOTTO_GDB_REGISTERS_H
#define LOTTO_GDB_REGISTERS_H

#include <stdint.h>

int64_t gdb_srv_handle_g(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_p(int fd, uint8_t *msg, uint64_t msg_len);

#endif // LOTTO_GDB_REGISTERS_H
