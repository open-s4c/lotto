/*
 */
#include <stdint.h>
#include <string.h>

#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/gdb_base.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/handling/execute.h>
#include <lotto/qlotto/gdb/handling/memory.h>
#include <lotto/qlotto/gdb/handling/query.h>
#include <lotto/qlotto/gdb/handling/registers.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/unsafe/_sys.h>

// See
// https://sourceware.org/gdb/current/onlinedocs/gdb.html/Packets.html#Packets

int64_t
gdb_srv_handle_msg(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg != NULL);
    ASSERT(msg_len > 0);

    // rl_fprintf(stderr, "Received msg from gdb client:\n%s\n", msg);

    uint8_t cmd_chr = 0;

    cmd_chr = msg[0];

    switch (cmd_chr) {
        case 'c':
            return gdb_srv_handle_c(fd, msg, msg_len);
            break;
        case 'D':
            return gdb_srv_handle_D(fd, msg, msg_len);
            break;
        case 'g':
            return gdb_srv_handle_g(fd, msg, msg_len);
            break;
        case 'H':
            return gdb_srv_handle_H(fd, msg, msg_len);
            break;
        case 'm':
            return gdb_srv_handle_m(fd, msg, msg_len);
            break;
        case 'p':
            return gdb_srv_handle_p(fd, msg, msg_len);
            break;
        case 'q':
            return gdb_srv_handle_q(fd, msg, msg_len);
            break;
        case 's':
            return gdb_srv_handle_s(fd, msg, msg_len);
            break;
        case 'T':
            return gdb_srv_handle_T(fd, msg, msg_len);
            break;
        case 'v':
            return gdb_srv_handle_v(fd, msg, msg_len);
            break;
        case 'z':
            return gdb_srv_handle_z(fd, msg, msg_len);
            break;
        case 'Z':
            return gdb_srv_handle_Z(fd, msg, msg_len);
            break;
        case '?':
            return gdb_srv_handle_questionmark(fd, msg, msg_len);
            break;
        default:
            rl_fprintf(stderr, "GDB server cannot handle msg:\n%s\n", msg);
            ASSERT(0 && "RSP srv command not implemented");
    }
    return 0;
}

int64_t
gdb_srv_handle_pkt(int fd, uint8_t *pkt, uint64_t pkt_len)
{
    ASSERT(pkt != NULL);
    ASSERT(pkt_len > 0);

    uint8_t msg[GDB_MAX_PACKET_LENGTH];

    uint64_t msg_len = rsp_extract_pkt(msg, pkt, &pkt_len);
    return gdb_srv_handle_msg(fd, msg, msg_len);
}

void
gdb_srv_stub_init(void)
{
}
