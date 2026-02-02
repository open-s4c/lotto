#ifndef LOTTO_GDB_EXECUTE_H
#define LOTTO_GDB_EXECUTE_H

#include <stdint.h>

int64_t gdb_srv_handle_ctrl_c(int fd);
int64_t gdb_srv_handle_c(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_D(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_H(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_v(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_s(int fd, uint8_t *msg, uint64_t msg_len);

int64_t gdb_server_set_hook_si(void);
int64_t gdb_server_reset_hook_si(void);

#endif // LOTTO_GDB_EXECUTE_H
