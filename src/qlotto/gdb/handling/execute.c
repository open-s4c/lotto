#include <stdint.h>
#include <string.h>

#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/handling/stop_reason.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/qlotto/gdb/util.h>
#include <lotto/unsafe/_sys.h>
#include <lotto/util/casts.h>

bool hook_si = false;

int64_t
gdb_server_set_hook_si(void)
{
    hook_si = true;
    return 0;
}

int64_t
gdb_server_reset_hook_si(void)
{
    hook_si = false;
    return 0;
}

int64_t
gdb_srv_handle_v(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg);
    ASSERT(msg[0] == 'v');

    uint64_t msg_idx = 0;

    // skip the 'v'
    msg_idx++;

    // MustReplyEmpty message
    if (msg_len >= rl_strlen("MustReplyEmpty") &&
        rl_strncmp((char *)msg + msg_idx, "MustReplyEmpty",
                   rl_strlen("MustReplyEmpty")) == 0) {
        gdb_send_ack(fd);
        gdb_send_empty(fd);
        return 0;
    }

    // vCont? support
    if (msg_len == rl_strlen("vCont?") &&
        strcmp((char *)msg + msg_idx, "Cont?") == 0) {
        gdb_send_ack(fd);
        gdb_send_msg(fd, (uint8_t *)"vCont;c;C;s;S;t;r", 17);
        return 0;
    }
    if (msg_len > rl_strlen("vCont;") &&
        rl_strncmp((char *)msg + msg_idx, "Cont;", rl_strlen("Cont;")) == 0) {
        msg_idx += 5;
        int64_t pid = 0;
        int64_t tid = 0;

        // rl_fprintf(stderr, "vCont:\n%s\n", (char*)msg);

        switch (msg[msg_idx]) {
            case 'c':
                // skip "c:"
                msg_idx += 2;
                ASSERT(msg[msg_idx] == 'p');
                msg_idx++;

                rsp_extract_thread_id((char *)msg + msg_idx, msg_len - msg_idx,
                                      &pid, &tid);

                if (hook_si) {
                    // continue with temporary breakpoint, just do stepping
                    gdb_send_ack(fd);
                    gdb_send_ok(fd);
                    gdb_set_stop_signal(5);
                    gdb_set_stop_reason(STOP_REASON_swbreak, 0);
                    gdb_set_break_next(GDB_BREAK_ANY);
                    gdb_execution_run();
                    return 0;
                }
                // check tid and pid for -1, which means continue all
                // log_infof("vCont continue on pid %ld, tid %ld\n", pid,
                // tid);
                gdb_send_ack(fd);
                gdb_srv_set_current_core_id(0);
                gdb_execution_run();

                return 0;
                break;
            case 'C':
                // skip "c:"
                msg_idx += 2;
                ASSERT(msg[msg_idx] == 'p');
                msg_idx++;
                rsp_extract_thread_id((char *)msg + msg_idx, msg_len - msg_idx,
                                      &pid, &tid);

                // log_infof("vCont continue on pid %ld, tid %ld\n", pid,
                // tid);
                gdb_send_ack(fd);
                gdb_srv_set_current_core_id(0);
                gdb_execution_run();

                return 0;
                break;
            case 's': // stepping
            case 'S': // stepping
                // _state_gdb_server.current_core_id = 0;
                // skip "s:" or "S:""
                msg_idx += 2;
                ASSERT(msg[msg_idx] == 'p');
                msg_idx++;
                msg_idx += rsp_extract_thread_id((char *)msg + msg_idx,
                                                 msg_len - msg_idx, &pid, &tid);
                ASSERT(tid > 0 && tid <= (int64_t)gdb_get_num_cpus());
                bool step_any     = false;
                uint64_t s_cpuidx = gdb_srv_gdb_tid_to_vcpu(tid);

                if (msg_len > msg_idx + 3) {
                    ASSERT(msg[msg_idx] == ';');
                    msg_idx++;
                    if (msg[msg_idx] == 'c') {
                        step_any = true;
                    }
                }

                gdb_send_ack(fd);
                gdb_send_ok(fd);
                gdb_set_stop_signal(5);
                gdb_set_stop_reason(STOP_REASON_swbreak, 0);
                if (step_any) {
                    gdb_set_break_next(GDB_BREAK_ANY);
                } else {
                    gdb_set_break_next(CAST_TYPE(int64_t, s_cpuidx));
                }
                // engine_await_halted();
                gdb_execution_run();
                // gdb_srv_send_T_info(fd, 5);
                return 0;
                break;
            case 'r': // run until range exit
                // skip "r"
                msg_idx += 1;

                // there should be 2 addresses split by ',' for now assume them
                // to be 64bit, so 16 encoded ascii characters each
                // vCont;rffff8000800809b8,ffff8000800809c4

                ASSERT(
                    msg_len - msg_idx >=
                    33); // 32 chars for 2 addresses plus a commata in between
                ASSERT(msg[msg_idx + 16] == ',');

                char *sep_next = strchr((char *)msg + msg_idx, ',');
                ASSERT(sep_next != NULL);

                ASSERT(sep_next == (char *)msg + msg_idx + 16);

                char *addr1        = (char *)msg + msg_idx;
                uint64_t addr1_len = sep_next - (char *)msg - msg_idx;

                char *addr2        = (char *)msg + msg_idx + 17;
                uint64_t addr2_len = 16;

                uint64_t b_pc_1 = rsp_str_to_hex(addr1, addr1_len);
                uint64_t b_pc_2 = rsp_str_to_hex(addr2, addr2_len);

                ASSERT(b_pc_2 > b_pc_1);

                uint64_t size = b_pc_2 - b_pc_1;

                int32_t cpu_index = -1;

                msg_idx += 17;

                if (msg_len - msg_idx >= 5 && msg[msg_idx] == ':' &&
                    msg[msg_idx + 1] == 'p') {
                    // check if a specific task is given
                    int64_t pid = 0;
                    int64_t tid = 0;
                    msg_idx += 2;

                    rsp_extract_thread_id((char *)msg + msg_idx,
                                          msg_len - msg_idx, &pid, &tid);

                    cpu_index =
                        CAST_TYPE(int32_t, gdb_srv_gdb_tid_to_vcpu(tid));
                }
                // log_printf("Got request to add breakpoint at addr 0x%lx\n",
                // b_pc);
                gdb_add_breakpoint_range_exclude(b_pc_1, size, cpu_index);
                gdb_send_ack(fd);
                gdb_send_ok(fd);
                gdb_execution_run();

                return 0;
                break;
            default:
                log_infof("Cannot handle vCont; request:\n%s\n", msg);
                ASSERT(0 && "Unsupported vCont; request");
                break;
        }
    }

    // vCont; request

    log_infof("Cannot handle v message:\n%s\n", msg);
    ASSERT(0 && "Unsupported v message");

    return 0;
}

int64_t
gdb_srv_handle_H(int fd, uint8_t *msg, uint64_t msg_len)
{
    int64_t msg_idx = 0;
    ASSERT(msg);
    ASSERT(msg[0] == 'H');

    // skip the 'H'
    msg_idx++;

    // gp message
    if (msg_len >= rl_strlen("Hgp") &&
        rl_strncmp((char *)msg + msg_idx, "gp", rl_strlen("gp")) == 0) {
        msg_idx += 2;
        int64_t pid = -1;
        int64_t tid = -1;

        rsp_extract_thread_id((char *)msg + msg_idx, msg_len - msg_idx, &pid,
                              &tid);
        // log_infof("Asked to switch to pid %d, tid %d\n", pid, tid);

        if (tid > 0)
            gdb_srv_set_current_core_id(tid);

        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    // set thread for next continue/step message
    if (msg[msg_idx] == 'c') {
        msg_idx++;
        // continue all
        gdb_srv_set_current_core_id(0);
        if (msg_len == rl_strlen("-1") + msg_idx &&
            strcmp((char *)msg + msg_idx, "-1") == 0) {
            gdb_send_ack(fd);
            gdb_send_ok(fd);
            return 0;
        }

        // continue specific process.thread
        if (msg[msg_idx] == 'p') {
            int64_t pid = -1;
            int64_t tid = -1;
            msg_idx++;

            rsp_extract_thread_id((char *)msg + msg_idx, msg_len - msg_idx,
                                  &pid, &tid);

            gdb_send_ack(fd);
            gdb_send_ok(fd);
            return 0;
        }

        log_infof("Cannot handle H continue/step message:\n%s\n", msg);
        ASSERT(0 && "Unsupported H continue/step message");
    }

    log_infof("Cannot handle H message:\n%s\n", msg);
    ASSERT(0 && "Unsupported H message");

    return 0;
}

// continue (from current address or specific address)
int64_t
gdb_srv_handle_c(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg_len > 0);
    ASSERT(msg[0] == 'c');

    gdb_srv_set_current_core_id(0);

    // skip 'c'
    msg++;
    msg_len--;

    // continue from current address
    if (msg_len == 0) {
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        gdb_execution_run();
        return 0;
    }

    log_infof("Cannot handle c message with specific address:\n%s\n", msg);
    ASSERT(0 && "Unsupported c address message");
    return 0;
}

int64_t
gdb_srv_handle_ctrl_c(int fd)
{
    // log_infof("Handle Ctrl+c\n");
    // stop the system
    gdb_set_stop_signal(2);
    gdb_set_stop_reason(STOP_REASON_NONE, 0);
    gdb_set_break_next(GDB_BREAK_ANY);
    return 0;
}

// Disconnect from gdb client
int64_t
gdb_srv_handle_D(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg_len == 3);
    ASSERT(msg[0] == 'D');
    ASSERT(msg[1] == ';');
    ASSERT(msg[2] == '1');

    // log_infof("GDB client disconnected\n");

    gdb_send_ack(fd);
    gdb_send_ok(fd);
    gdb_srv_set_current_core_id(0);
    gdb_execution_run();

    return 0;
}

// continue (from current address or specific address)
int64_t
gdb_srv_handle_s(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg_len > 0);
    ASSERT(msg[0] == 's');

    gdb_set_stop_signal(5);
    gdb_set_stop_reason(STOP_REASON_swbreak, 0);
    gdb_set_break_next(GDB_BREAK_ANY);

    // skip 's'
    msg++;
    msg_len--;

    // continue from current address
    if (msg_len == 0) {
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        gdb_execution_run();
        return 0;
    }

    log_infof("Cannot handle c message with specific address:\n%s\n", msg);
    ASSERT(0 && "Unsupported c address message");
    return 0;
}
