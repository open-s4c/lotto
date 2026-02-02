#ifndef GDB_SERVER_STUB_H
#define GDB_SERVER_STUB_H

#include <qemu-plugin.h>
// #include <stdbool.h>
#include <stdint.h>

/************
 *   GDB    *
 ************/

int64_t gdb_srv_handle_msg(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_pkt(int fd, uint8_t *pkt, uint64_t pkt_len);

void gdb_srv_stub_init(void);

#endif // GDB_SERVER_STUB_H
