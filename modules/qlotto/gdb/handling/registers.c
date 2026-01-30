#include <stdint.h>
#include <string.h>

#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/gdb_base.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/qlotto/qemu/armcpu.h>
#include <lotto/unsafe/_sys.h>

static int64_t
gdb_srv_send_registers(int fd)
{
    // as described in AARCH64_CORE_XML
    // send value of registers x0-x30,sp,pc,cpsr
    // all 64bit values, except cpsr (32 bit)

    char msg_registers[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_idx = 0;
    uint64_t msg_len = 0;

    uint64_t cur_cpu = gdb_srv_get_cur_cpuindex();

    CPUARMState *parmcpu = gdb_get_parmcpu(cur_cpu);

    if (parmcpu == NULL) {
        // cpu structure / registers not known yet, send null values
        for (int i = 0; i <= 30; i++) // x0-x30
        {
            msg_idx += rsp_hex_to_str(msg_registers + msg_idx, 0, 16);
        }
        msg_idx += rsp_hex_to_str(msg_registers + msg_idx, 0, 16); // sp
        msg_idx += rsp_hex_to_str(msg_registers + msg_idx, 0, 16); // pc
        msg_idx += rsp_hex_to_str(msg_registers + msg_idx, 0, 8);  // cpsr
    } else {
        uint64_t reg_size   = 0;
        uint128_t reg_value = 0;
        for (int reg_num = 0; reg_num <= 33; reg_num++) {
            reg_size  = gdb_get_register_size(reg_num);
            reg_value = gdb_get_register_value(parmcpu, reg_num);

            // rl_fprintf(stderr, "Register %d: %lu\n", reg_num,
            // (uint64_t)reg_value);

            if (reg_size == 32) {
                msg_idx +=
                    rsp_hex_to_str(msg_registers + msg_idx,
                                   htobe32((uint32_t)reg_value), reg_size / 4);
            }

            if (reg_size == 64) {
                msg_idx +=
                    rsp_hex_to_str(msg_registers + msg_idx,
                                   htobe64((uint64_t)reg_value), reg_size / 4);
            }

            if (reg_size == 128) {
                msg_idx += rsp_hex_to_str(msg_registers + msg_idx,
                                          htobe128(reg_value), reg_size / 4);
            }
        }
    }
    msg_len                = msg_idx;
    msg_registers[msg_len] = '\0';

    // logger_infof("sending register values:\n%s\n", msg_registers);

    gdb_send_ack(fd);
    gdb_send_msg(fd, (uint8_t *)msg_registers, msg_len);
    return 0;
}

// send registers
int64_t
gdb_srv_handle_g(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg);
    ASSERT(msg[0] == 'g');

    // skip the 'g'
    msg++;
    msg_len--;

    if (msg_len == 0) {
        return gdb_srv_send_registers(fd);
    }

    // gp message
    if (msg_len >= rl_strlen("gp") &&
        rl_strncmp((char *)msg, "gp", rl_strlen("gp")) == 0) {
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    logger_infof("Cannot handle g message:\n%s\n", msg);
    ASSERT(0 && "Unsupported g message");

    return 0;
}

// read specific register
int64_t
gdb_srv_handle_p(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg);
    ASSERT(msg[0] == 'p');
    ASSERT(msg_len >= 2);

    uint64_t msg_idx = 0;
    uint8_t msg_answer[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_answer_len = 0;

    uint64_t cur_cpu = gdb_srv_get_cur_cpuindex();

    CPUARMState *parmcpu = gdb_get_parmcpu(cur_cpu);

    // skip the 'p'
    msg_idx++;

    uint64_t reg_num = rsp_str_to_hex((char *)msg + msg_idx, msg_len - 1);

    uint64_t reg_size   = gdb_get_register_size(reg_num);
    uint128_t reg_value = gdb_get_register_value(parmcpu, reg_num);

    if (reg_size == 32) {
        msg_answer_len = rsp_hex_to_str(
            (char *)msg_answer, htobe32((uint32_t)reg_value), reg_size / 4);
    }

    if (reg_size == 64) {
        msg_answer_len = rsp_hex_to_str(
            (char *)msg_answer, htobe64((uint64_t)reg_value), reg_size / 4);
    }

    if (reg_size == 128) {
        msg_answer_len = rsp_hex_to_str((char *)msg_answer, htobe128(reg_value),
                                        reg_size / 4);
    }

    if (msg_answer_len > 0) {
        msg_answer[msg_answer_len] = '\0';

        gdb_send_ack(fd);
        gdb_send_msg(fd, msg_answer, msg_answer_len);
    } else {
        logger_infof("Unknown register %lu\n", reg_num);
        ASSERT(0 && "Unknown register number.");
    }

    return 0;
}
