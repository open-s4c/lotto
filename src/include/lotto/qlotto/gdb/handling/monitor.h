#ifndef LOTTO_GDB_MONITOR_H
#define LOTTO_GDB_MONITOR_H

#include <stdint.h>

int64_t gdb_srv_handle_Rcmd(int fd, uint8_t *msg, uint64_t msg_len);

#endif // LOTTO_GDB_MONITOR_H
