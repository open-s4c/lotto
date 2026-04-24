#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

static int
connect_range_retry(const char *host, uint16_t port, uint16_t span,
                    int timeout_ms)
{
    struct sockaddr_in addr = {0};
    addr.sin_family         = AF_INET;
    if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
        fprintf(stderr, "qemu-gdb-rsp: bad host '%s'\n", host);
        return -1;
    }

    const int step_ms = 100;
    for (int elapsed = 0; elapsed < timeout_ms; elapsed += step_ms) {
        for (uint16_t off = 0; off < span; ++off) {
            int fd = socket(AF_INET, SOCK_STREAM, 0);
            if (fd < 0) {
                perror("socket");
                return -1;
            }
            addr.sin_port = htons((uint16_t)(port + off));
            if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
                fprintf(stderr, "qemu-gdb-rsp: connected to %u\n",
                        (unsigned)(port + off));
                return fd;
            }
            close(fd);
        }

        struct timespec ts = {
            .tv_sec  = 0,
            .tv_nsec = step_ms * 1000 * 1000,
        };
        nanosleep(&ts, NULL);
    }

    fprintf(stderr, "qemu-gdb-rsp: connect timeout\n");
    return -1;
}

static int
recv_byte(int fd, char *out)
{
    ssize_t n = recv(fd, out, 1, 0);
    return n == 1 ? 0 : -1;
}

static int
recv_packet(int fd, char *buf, size_t cap)
{
    char ch = 0;
    do {
        if (recv_byte(fd, &ch) != 0)
            return -1;
    } while (ch == '+');

    if (ch != '$')
        return -1;

    size_t len   = 0;
    uint8_t csum = 0;
    for (;;) {
        if (recv_byte(fd, &ch) != 0)
            return -1;
        if (ch == '#')
            break;
        if (len + 1 >= cap)
            return -1;
        buf[len++] = ch;
        csum += (uint8_t)ch;
    }

    char hex[2];
    if (recv(fd, hex, 2, MSG_WAITALL) != 2)
        return -1;

    static const char h[] = "0123456789abcdef";
    char want[2]          = {h[(csum >> 4) & 0xf], h[csum & 0xf]};
    if (hex[0] != want[0] || hex[1] != want[1])
        return -1;

    if (send(fd, "+", 1, 0) != 1)
        return -1;

    buf[len] = '\0';
    return (int)len;
}

static int
send_packet(int fd, const char *msg)
{
    static const char h[] = "0123456789abcdef";
    uint8_t csum          = 0;
    for (const unsigned char *p = (const unsigned char *)msg; *p; ++p)
        csum += *p;

    char hdr[4] = {'$', 0, '#', 0};
    (void)hdr;
    if (send(fd, "$", 1, 0) != 1)
        return -1;
    if (send(fd, msg, strlen(msg), 0) != (ssize_t)strlen(msg))
        return -1;
    char tail[3] = {'#', h[(csum >> 4) & 0xf], h[csum & 0xf]};
    if (send(fd, tail, 3, 0) != 3)
        return -1;

    char ack = 0;
    if (recv_byte(fd, &ack) != 0 || ack != '+')
        return -1;
    return 0;
}

int
main(int argc, char **argv)
{
    const char *host = "127.0.0.1";
    uint16_t port    = 12255;
    if (argc >= 2)
        host = argv[1];
    if (argc >= 3)
        port = (uint16_t)strtoul(argv[2], NULL, 10);

    int fd = connect_range_retry(host, port, 256, 5000);
    if (fd < 0)
        return 1;

    char buf[8192];

    if (send_packet(fd, "?") != 0 || recv_packet(fd, buf, sizeof(buf)) <= 0) {
        fprintf(stderr, "qemu-gdb-rsp: failed status query\n");
        close(fd);
        return 1;
    }

    if (send_packet(fd, "g") != 0 || recv_packet(fd, buf, sizeof(buf)) <= 0) {
        fprintf(stderr, "qemu-gdb-rsp: failed register read\n");
        close(fd);
        return 1;
    }

    if (send_packet(fd, "c") != 0 || recv_packet(fd, buf, sizeof(buf)) <= 0) {
        fprintf(stderr, "qemu-gdb-rsp: failed continue\n");
        close(fd);
        return 1;
    }

    fprintf(stderr, "qemu-gdb-rsp: ok\n");
    close(fd);
    return 0;
}
