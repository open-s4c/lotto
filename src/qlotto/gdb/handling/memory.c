#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/gdb_base.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/unsafe/_sys.h>

int cpu_memory_rw_debug(/*CPUState*/ void *cpu, uint64_t addr, void *ptr,
                        size_t len, bool is_write);

// read memory
int64_t
gdb_srv_handle_m(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint8_t memory_value[8192];
    uint8_t msg_answer[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_answer_idx = 0;
    uint64_t msg_answer_len = 0;

    uint64_t msg_idx = 0;

    int64_t rc = 0;

    uint64_t mem_addr = 0;
    // uint64_t mem_value = 0;
    uint64_t mem_size = 0; // in bytes

    uint64_t cur_cpu = gdb_srv_get_cur_cpuindex();

    void *cpu = gdb_get_pcpu(cur_cpu);

    ASSERT(msg[0] == 'm');
    ASSERT(msg_len > 1);

    // skip 'm'
    msg_idx = 1;

    char *sep_ptr = strchr((char *)msg + msg_idx, ',');
    ASSERT(sep_ptr != NULL);

    uint64_t addr_len = sep_ptr - (char *)msg - msg_idx;
    ASSERT(addr_len >= 1);

    mem_addr = rsp_str_to_hex((char *)msg + msg_idx, addr_len);
    msg_idx += addr_len;

    ASSERT(msg[msg_idx] == ',');
    msg_idx++;

    mem_size = rsp_str_to_hex((char *)msg + msg_idx, msg_len - msg_idx);
    // ASSERT(mem_size % 4 == 0);
    ASSERT(mem_size <= 8192);
    // logger_infof("Reading %d bytes from 0x%lx\n", mem_size, mem_addr);

    if (mem_addr == 0 || mem_addr > 0xfffffffff0000000) {
        gdb_send_ack(fd);
        gdb_send_error(fd, 0x14);
        return 0;
        // rl_memset(memory_value, 0, mem_size);
    } else {
        rc = cpu_memory_rw_debug(cpu, mem_addr, memory_value, mem_size, 0);
        // ASSERT(rc == 0);
        if (rc != 0) {
            rl_memset(memory_value, 0, mem_size);
        }
    }

    for (uint64_t i = 0; i < mem_size; i++) {
        msg_answer_idx += rsp_hex_to_str((char *)msg_answer + msg_answer_idx,
                                         0xFF & memory_value[i], 2);
    }

    msg_answer_len             = msg_answer_idx;
    msg_answer[msg_answer_len] = '\0';
    gdb_send_ack(fd);
    return gdb_send_msg(fd, msg_answer, msg_answer_len);
}
