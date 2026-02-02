#ifndef LOTTO_GDB_FD_STUB_H
#define LOTTO_GDB_FD_STUB_H

int fd_stub_register(int tmp_fd);
int fd_stub_deregister(int fd);
void fd_stub_init(void);

#endif // LOTTO_GDB_FD_STUB_H
