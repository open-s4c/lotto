/*
 */
#ifndef LOTTO_QEMU_GDB_SEND_H
#define LOTTO_QEMU_GDB_SEND_H

#include <stdint.h>

#define GDB_MAX_QEMU_THREADS  1024
#define GDB_PACKET_QUEUE_SIZE 1000

int64_t gdb_has_pkt_queued();
int64_t gdb_send_next_pkt();
int64_t gdb_send_msg(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_send_pkt(int fd, uint8_t *pkt, uint64_t pkt_len);
int64_t gdb_send_ok(int fd);
int64_t gdb_send_error(int fd, uint32_t error_num);
int64_t gdb_send_list_end(int fd);
int64_t gdb_send_one(int fd);
int64_t gdb_send_zero(int fd);
int64_t gdb_send_empty(int fd);
int64_t gdb_send_ack(int fd);
int64_t gdb_send_retry(int fd);
int64_t gdb_send_ctrl_c(int fd);
int64_t gdb_send_interrupt(int fd, int64_t pid, int64_t tid);

int64_t gdb_srv_save_last_send_pkt(uint8_t *pkt, uint64_t pkt_len);
int64_t gdb_srv_resend_last_pkt(int fd);

void gdb_send_init(void);

#endif // GDB_SEND_H
