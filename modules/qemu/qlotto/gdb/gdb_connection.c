/*
 */

#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <linux/tcp.h>
#include <lotto/qlotto/gdb/fd_stub.h>
#include <lotto/qlotto/gdb/gdb_base.h>
#include <lotto/qlotto/gdb/gdb_connection.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/gdb_server_stub.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/handling/execute.h>
#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/unsafe/_sys.h>

static struct gdb_srv_connection {
    struct pollfd fds[GDB_MAX_CONNECTIONS + 2];
    int nfds;

    uint64_t recv_buffer_host_idx;
    char recv_buffer_host[GDB_MAX_PACKET_LENGTH];

    uint64_t recv_buffer_cli_idx;
    char recv_buffer_cli[GDB_MAX_PACKET_LENGTH];

    int64_t await_host_ack;
    int gdb_client_fd;
} _state;


int64_t
gdb_proxy_close_fds()
{
    for (; _state.nfds >= 0; _state.nfds--) {
        close(_state.fds[_state.nfds].fd);
        _state.fds[_state.nfds].fd = 0;
    }
    return 0;
}

int64_t
gdb_is_host_fd(int fd)
{
    return (fd == _state.fds[1].fd);
}

bool
gdb_srv_has_client_connected()
{
    return _state.gdb_client_fd != 0;
}

void
gdb_srv_set_client_fd(int fd)
{
    _state.gdb_client_fd = fd;
}

int
gdb_srv_get_client_fd()
{
    return _state.gdb_client_fd;
}

int64_t
gdb_srv_listen(struct pollfd *fds, int *nfds)
{
    struct sockaddr_in addr;

    fds->fd = rl_socket(AF_INET, SOCK_STREAM, 0);
    if (fds->fd == -1) {
        rl_printf("Error opening socket for gdb_server\n");
        exit(1);
    }

    // keep open FDs constant and identical numbers
    fds->fd = fd_stub_register(fds->fd);

    int optval = 1;
    if (rl_setsockopt(fds->fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                      sizeof(optval)) < 0) {
        rl_printf("Set sockopt fail.\n");
        exit(1);
    }

    uint16_t port_offset = 0;

    bool success_bind = false;

    for (port_offset = 0; port_offset < 32768; port_offset++) {
        addr.sin_port        = htons(GDB_SERVER_PORT) + port_offset;
        addr.sin_addr.s_addr = 0;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_family      = AF_INET;

        if (rl_bind(fds->fd, (struct sockaddr *)&addr,
                    sizeof(struct sockaddr_in)) == 0) {
            success_bind = true;
            break;
        }
    }

    if (!success_bind) {
        logger_infof("Error binding socket for gdb_server\n");
        exit(1);
    }

    if (rl_listen(fds->fd, 1) < 0) {
        perror("listen");
        exit(1);
    }

    logger_infof("Successfully started up gdb_server to port %u\n", addr.sin_port);

    fds->events = POLLIN;
    (*nfds)++;

    return 0;
}

void qemu_gdb_enable(void);
bool qemu_gdb_get_wait(void);
void qemu_gdb_set_wait(bool wait);

int64_t
gdb_srv_accept(struct pollfd *fds, int *nfds)
{
    int optval = 1;

    if ((*nfds - 2) < GDB_MAX_CONNECTIONS) {
        (*nfds)++;

        int tmp_fd    = rl_accept(fds[0].fd, NULL, NULL);
        tmp_fd        = fd_stub_register(tmp_fd);
        fds[*nfds].fd = tmp_fd;

        fds[*nfds].events = POLLIN;
        // logger_infof( "Accepted client connection.\n");

        // Enable TCP keep alive process
        optval = 1;
        rl_setsockopt(fds[*nfds].fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&optval,
                      sizeof(optval));

        // Don't delay small packets, for better interactive
        // response (disable Nagel's algorithm)
        optval = 1;
        rl_setsockopt(fds[*nfds].fd, IPPROTO_TCP, TCP_NODELAY, (char *)&optval,
                      sizeof(optval));

        qemu_gdb_enable();

        if (!qemu_gdb_get_wait()) {
            gdb_set_break_next(GDB_BREAK_ANY);
            gdb_set_stop_signal(5);
            gdb_set_stop_reason(STOP_REASON_NONE, 0);
        } else {
            qemu_gdb_set_wait(false);
        }
        gdb_srv_set_client_fd(fds[*nfds].fd);

        (*nfds)++;

    } else // no more connections allowed
    {
        ASSERT(0 && "Too many connections to gdb-server.");
        int tmp_fd = rl_accept(fds[0].fd, NULL, NULL);
        close(tmp_fd);
    }
    return 0;
}

int64_t
gdb_awaits_host_inc()
{
    _state.await_host_ack++;
    return 0;
}

int64_t
gdb_awaits_host_dec()
{
    _state.await_host_ack--;
    return 0;
}

int64_t
gdb_awaits_host_ack()
{
    ASSERT(_state.await_host_ack >= 0);
    return _state.await_host_ack;
}

int64_t
gdb_srv_recv_pkt(struct pollfd *fds, int *nfds)
{
    char buffer[GDB_MAX_PACKET_LENGTH];
    uint64_t pkt_len = 0;
    uint64_t pkt_idx = 0;
    int64_t rc;

    rc         = recv(fds->fd, buffer, sizeof(buffer), 0);
    buffer[rc] = '\0';
    // logger_infof( "Received pkt from gdb client:\n%s\n", buffer);

    // connection closed
    if (rc == 0) {
        fds->fd     = fd_stub_deregister(fds->fd);
        fds->fd     = 0;
        fds->events = 0;
        (*nfds)--;
    }

    pkt_len = rc;

    if (rc < 0)
        return rc;

    while (pkt_idx < pkt_len) {
        switch (buffer[pkt_idx]) {
            case 0x03:
                gdb_srv_handle_ctrl_c(fds->fd);
                pkt_idx++;
                break;
            case '+':
                pkt_idx++;
                break;
            case '-':
                gdb_srv_resend_last_pkt(fds->fd);
                pkt_idx++;
                break;
            case '$': {
                uint64_t actual_pkt_len = pkt_len - pkt_idx;
                rc = gdb_srv_handle_pkt(fds->fd, (uint8_t *)buffer + pkt_idx,
                                        actual_pkt_len);
                pkt_idx += actual_pkt_len;
            } break;
            default:
                logger_infof(
                    "unexpected char %c (hex %x) at idx %lu in pkt (len: "
                    "%lu):\n%s\n",
                    buffer[pkt_idx], buffer[pkt_idx], pkt_idx, pkt_len, buffer);
                ASSERT(0 && "Error parsing packet from gdb client");
                break;
        }
    }

    return 0;
}

int64_t
gdb_srv_poll_fds()
{
    int64_t rc = 0;

    // logger_infof( "Polling %d fds.\n", nfds);
    rc = rl_poll(_state.fds, _state.nfds, 100);

    for (int i = 0; i < _state.nfds; i++) {
        if (_state.fds[i].revents == 0)
            continue;

        // on listen socket, accept a new connection
        if (i == 0) {
            if (_state.fds[0].revents & POLLIN) {
                rc = gdb_srv_accept(_state.fds, &_state.nfds);
            }
        } else // from a gdb client connected to the proxy
        {
            if (_state.fds[i].revents & POLLIN) {
                gdb_srv_recv_pkt(&(_state.fds[i]), &_state.nfds);
            }
        }
    }
    return rc;
}

int64_t
gdb_srv_connection_init(void)
{
    rl_memset(_state.fds, 0, sizeof(_state.fds));
    gdb_srv_listen(_state.fds, &_state.nfds);
    return 0;
}
