/*
 */

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lotto/qlotto/gdb/gdb_connection.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/sys/logger.h>
#include <lotto/sys/time.h>
#include <lotto/unsafe/rogue.h>
#include <vsync/atomic/core.h>

#define INIT   0
#define INITED 1

bool qemu_gdb_enabled(void);

vatomic32_t server_thread_created;

void *
gdb_server_thread(void *data)
{
    vatomic32_write(&server_thread_created, INITED);
    lotto_rogue({
        while (1) {
            if (gdb_execution_has_halted()) {
                gdb_srv_handle_halted();
            }
            gdb_srv_poll_fds();
            while (gdb_has_pkt_queued()) {
                gdb_send_next_pkt();
            }
        }
        gdb_proxy_close_fds();
    });
    return NULL;
}

int
gdb_server_start(void)
{
    pthread_t thid_gdb_server;
    vatomic32_init(&server_thread_created, INIT);

    if (pthread_create(&thid_gdb_server, 0, gdb_server_thread, NULL) != 0) {
        logger_fatalf("pthread_create() error on gdb_server_thread\n");
        exit(1);
    }
    while (vatomic32_read(&server_thread_created) == 0) {
        sched_yield();
    };
    return 0;
}
