#ifndef GDB_CONNECTION_H
#define GDB_CONNECTION_H

#include <stdint.h>

#define GDB_SERVER_PORT     12255
#define GDB_MAX_CONNECTIONS 4

#define GDB_HOST_SERVER_PORT 12244

int64_t gdb_is_host_fd(int fd);

int64_t gdb_srv_connection_init();
int64_t gdb_proxy_close_fds();
int64_t gdb_srv_poll_fds();

int64_t gdb_awaits_host_inc();
int64_t gdb_awaits_host_dec();
int64_t gdb_awaits_host_ack();

int gdb_srv_get_client_fd();

#endif // GDB_CONNECTION_H
