/*
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <lotto/base/map.h>
#include <lotto/qlotto/gdb/gdb_base.h>
#include <lotto/qlotto/gdb/gdb_connection.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/unsafe/_sys.h>

typedef struct packet_queue_s {
    mapitem_t ti;
    uint64_t pkt_len;
    int32_t fd;
    uint8_t pkt[GDB_MAX_PACKET_LENGTH];
} packet_queue_t;

static struct gdb_srv_send {
    map_t packet_queue;
    uint64_t packet_num_enqueue;
    uint64_t packet_num_dequeue;
    uint8_t srv_last_send_pkt[GDB_MAX_PACKET_LENGTH];
    uint64_t srv_last_send_pkt_len;
} _state;

int64_t
gdb_dequeue_pkt()
{
    int64_t rc = 0;

    if (map_size(&_state.packet_queue) == 0)
        return 0;

    _state.packet_num_dequeue++;

    packet_queue_t *dequeued_pkt = (packet_queue_t *)map_find(
        &_state.packet_queue, _state.packet_num_dequeue);

    ASSERT(NULL != dequeued_pkt);

    // logger_infof( "Sending packet:\n%s\n", dequeued_pkt->pkt);

    rc = write(dequeued_pkt->fd, dequeued_pkt->pkt, dequeued_pkt->pkt_len);

    rc = gdb_srv_save_last_send_pkt(dequeued_pkt->pkt, dequeued_pkt->pkt_len);

    map_deregister(&_state.packet_queue, _state.packet_num_dequeue);
    return rc;
}

int64_t
gdb_has_pkt_queued()
{
    return map_size(&_state.packet_queue) > 0;
}

int64_t
gdb_send_next_pkt()
{
    return gdb_dequeue_pkt();
}

static int64_t
gdb_enqueue_pkt(int fd, uint8_t *pkt, uint64_t pkt_len)
{
    ASSERT(pkt_len < GDB_MAX_PACKET_LENGTH && pkt_len > 0);

    _state.packet_num_enqueue++;
    packet_queue_t *enqueued_pkt = (packet_queue_t *)map_register(
        &_state.packet_queue, _state.packet_num_enqueue);
    enqueued_pkt->fd      = fd;
    enqueued_pkt->pkt_len = pkt_len;
    rl_memcpy(enqueued_pkt->pkt, pkt, pkt_len);

    if (pkt_len > 1) {
        gdb_awaits_host_inc();
    }
    return 0;
}

int64_t
gdb_send_pkt(int fd, uint8_t *pkt, uint64_t pkt_len)
{
    int64_t rc = 0;

    rc = gdb_enqueue_pkt(fd, pkt, pkt_len);

    return rc;
}

int64_t
gdb_send_msg(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint8_t packet[GDB_MAX_PACKET_LENGTH];
    uint64_t pkt_len = 0;

    pkt_len = rsp_construct_pkt(packet, msg, msg_len);

    return gdb_send_pkt(fd, packet, pkt_len);
}

int64_t
gdb_send_ok(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"$OK#9a", 6);
}

int64_t
gdb_send_error(int fd, uint32_t error_num)
{
    uint8_t msg[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_idx = 0;
    uint64_t msg_len = 0;

    msg[msg_idx] = 'E';
    msg_idx++;

    msg_idx += rsp_hex_to_str((char *)msg + msg_idx, error_num, 0);
    msg_len      = msg_idx;
    msg[msg_len] = '\0';

    return gdb_send_msg(fd, msg, msg_len);
}

int64_t
gdb_send_one(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"$1#31", 5);
}

int64_t
gdb_send_zero(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"$0#00", 4); // TODO: wrong Checksum!
}

int64_t
gdb_send_empty(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"$#00", 4);
}

int64_t
gdb_send_list_end(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"$l#6c", 5);
}

int64_t
gdb_send_ack(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"+", 1);
}

int64_t
gdb_send_retry(int fd)
{
    return gdb_send_pkt(fd, (uint8_t *)"-", 1);
}

int64_t
gdb_send_interrupt(int fd, int64_t pid, int64_t tid)
{
    char msg[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_idx = 0;
    uint64_t msg_len = 0;

    msg_idx = rl_snprintf(msg, GDB_MAX_MESSAGE_LENGTH, "vCont;t:p");
    msg_idx += rsp_construct_thread_id(msg + msg_idx, pid, tid);

    msg[msg_idx] = '\0';
    msg_len      = msg_idx;
    return gdb_send_msg(fd, (uint8_t *)msg, msg_len);
}

int64_t
gdb_srv_save_last_send_pkt(uint8_t *pkt, uint64_t pkt_len)
{
    ASSERT(pkt_len < GDB_MAX_PACKET_LENGTH);
    rl_memcpy(_state.srv_last_send_pkt, pkt, pkt_len);
    _state.srv_last_send_pkt[pkt_len] = '\0';
    _state.srv_last_send_pkt_len      = pkt_len;
    return 0;
}

int64_t
gdb_srv_resend_last_pkt(int fd)
{
    ASSERT(_state.srv_last_send_pkt_len > 0);
    // logger_infof("resend last packet:\n%s\n", srv_last_send_pkt);
    gdb_send_pkt(fd, _state.srv_last_send_pkt, _state.srv_last_send_pkt_len);
    return 0;
}

void
gdb_send_init(void)
{
    map_init(&_state.packet_queue, MARSHABLE_STATIC(sizeof(packet_queue_t)));
}
