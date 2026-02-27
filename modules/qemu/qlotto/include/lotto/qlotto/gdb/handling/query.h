#ifndef LOTTO_GDB_QUERY_H
#define LOTTO_GDB_QUERY_H

#include <stdint.h>

int64_t gdb_srv_handle_q(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_send_T_info(int fd, uint64_t halted_sig);
int64_t gdb_srv_handle_questionmark(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_T(int fd, uint8_t *msg, uint64_t msg_len);

#endif // LOTTO_GDB_QUERY_H
