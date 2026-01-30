#include <blob-aarch64-core.h>
#include <blob-aarch64-fpu.h>
#include <blob-system-registers.h>
#include <blob-target.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lotto/qlotto/gdb/arm_cpu.h>
#include <lotto/qlotto/gdb/gdb_base.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/halter.h>
#include <lotto/qlotto/gdb/handling/monitor.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/unsafe/_sys.h>

static struct query_state {
    int64_t ti_idx;
} _state;

static int64_t
gdb_srv_handle_xfer(int fd, uint8_t *xfer, uint64_t msg_len)
{
    ASSERT(xfer);
    ASSERT(msg_len > 0);

    char *read_file;

    // logger_infof("Got Xfer request: %s\n", xfer);

    // feature request
    if (rl_strncmp((char *)xfer,
                   "features:read:", rl_strlen("features:read:")) == 0) {
        char *fread = (char *)xfer + rl_strlen("features:read:");

        char annex[1024];
        uint64_t annex_len = 0;

        uint64_t read_offset = 0;
        char *offset;
        uint64_t offset_len;

        uint64_t read_length = 0;
        char *length;
        uint64_t length_len;

        // get annex from xfer request
        char *annex_end = strchr(fread, ':');
        ASSERT(annex_end != NULL);
        annex_len = annex_end - fread;
        strncpy(annex, fread, annex_len);
        annex[annex_len] = '\0';

        // get read offset from xfer request
        offset           = fread + annex_len + 1;
        char *offset_end = strchr(offset, ',');
        ASSERT(offset_end != NULL);
        offset_len  = offset_end - offset;
        read_offset = rsp_str_to_hex(offset, offset_len);

        // get read length from xfer request
        length           = offset + offset_len + 1;
        char *length_end = strchr(length, '\0');
        ASSERT(length_end != NULL);
        length_len  = length_end - length;
        read_length = rsp_str_to_hex(length, length_len);

        if (strcmp(annex, "target.xml") == 0) {
            // logger_infof("  target.xml\n");
            read_file = (char *)target_xml;
        }

        if (strcmp(annex, "aarch64-core.xml") == 0) {
            // logger_infof("  aarch64-core.xml\n");
            read_file = (char *)aarch64_core_xml;
        }

        if (strcmp(annex, "aarch64-fpu.xml") == 0) {
            // logger_infof("  aarch64-fpu.xml\n");
            read_file = (char *)aarch64_fpu_xml;
        }

        if (strcmp(annex, "system-registers.xml") == 0) {
            // logger_infof("  system-registers.xml\n");
            read_file = (char *)system_registers_xml;
        }

        ASSERT(read_file != NULL);
        ASSERT(rl_strlen(read_file) > read_offset);

        char *read_file_start  = read_file + read_offset;
        uint64_t read_file_len = rl_strlen(read_file_start);

        char list_chr = 'l';
        // if we need to send more than was requested, use 'm' instead of 'l'
        if (read_file_len > read_length)
            list_chr = 'm';

        gdb_send_ack(fd);

        char msg_answer[8192];
        msg_answer[0] = list_chr;
        strncpy(msg_answer + 1, read_file_start,
                MIN(read_file_len, read_length));
        uint64_t msg_answer_len = 1 + MIN(read_file_len, read_length);
        msg_answer[1 + MIN(read_file_len, read_length)] = '\0';

        // logger_infof("\nsending msg answer:\n%s\n", msg_answer);

        return gdb_send_msg(fd, (uint8_t *)msg_answer, msg_answer_len);
    }

    logger_infof("Client message: %s\n", (char *)xfer);
    ASSERT(0 && "Unsupported Xfer request");
    return -1;
}

static int64_t
gdb_srv_handle_supported(int fd, uint8_t *supported, uint64_t msg_len)
{
    ASSERT(supported);
    ASSERT(msg_len > 0);

    char *ptok = NULL;
    char *support_answer =
        "PacketSize=1000;qXfer:features:read+;vContSupported+;multiprocess+;"
        "swbreak+;hwbreak+";
    uint8_t support_packet[GDB_MAX_PACKET_LENGTH];
    uint64_t pkt_len = 0;

    // logger_infof("Got Supported info:\n");

    ptok = strtok((char *)supported, ";");
    while (ptok != NULL) {
        // logger_infof("  %s\n", ptok);
        ptok = strtok(NULL, ";");
    }

    gdb_send_ack(fd);

    pkt_len = rsp_construct_pkt(support_packet, (uint8_t *)support_answer,
                                rl_strlen(support_answer));
    gdb_send_pkt(fd, support_packet, pkt_len);

    return 0;
}

static int64_t
gdb_srv_handle_thread_info(int fd, uint8_t *msg, uint64_t msg_len)
{
    char msg_answer[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_answer_idx = 0;
    uint64_t msg_answer_len = 0;
    int64_t pid             = 1;

    gdb_send_ack(fd);

    // start listing threads
    if (msg[0] == 'f') {
        _state.ti_idx = 0;
    }

    if (_state.ti_idx == CAST_TYPE(int64_t, gdb_get_num_cpus())) {
        gdb_send_list_end(fd);
        return 0;
    }

    msg_answer[msg_answer_idx] = 'm';
    msg_answer_idx++;

    msg_answer[msg_answer_idx] = 'p';
    msg_answer_idx++;

    msg_answer_idx += rsp_construct_thread_id(
        (char *)msg_answer + msg_answer_idx, pid, ++_state.ti_idx);

    msg_answer_len = msg_answer_idx;

    msg_answer[msg_answer_len] = '\0';

    gdb_send_msg(fd, (uint8_t *)msg_answer, msg_answer_len);

    return 0;
}

static int64_t
gdb_srv_handle_thread_extra_info(int fd, uint8_t *msg, uint64_t msg_len)
{
    char msg_answer[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_idx = 0;

    uint64_t msg_answer_idx = 0;
    uint64_t msg_answer_len = 0;
    int64_t pid             = 0;
    int64_t tid             = 0;

    ASSERT(msg_len > rl_strlen("ThreadExtraInfo"));
    ASSERT(rl_strncmp((char *)msg, "ThreadExtraInfo",
                      rl_strlen("ThreadExtraInfo")) == 0);
    msg_idx += rl_strlen("ThreadExtraInfo");

    ASSERT(msg[msg_idx] == ',');
    msg_idx++;
    ASSERT(msg[msg_idx] == 'p');
    msg_idx++;

    gdb_send_ack(fd);

    ASSERT(msg_len - msg_idx == 3);

    rsp_extract_thread_id((char *)msg + msg_idx, msg_len - msg_idx, &pid, &tid);

    ASSERT(pid == 1);

    char ascii_cpu_num = CAST_TYPE(char, rsp_hex_to_chr(tid));
    msg_answer_idx += rsp_ascii_to_hex(msg_answer + msg_answer_idx, "(CPU#",
                                       rl_strlen("(CPU#"));
    msg_answer_idx +=
        rsp_ascii_to_hex(msg_answer + msg_answer_idx, &ascii_cpu_num, 1);

    if (tid == gdb_get_execution_halted_gdb_tid()) {
        msg_answer_idx += rsp_ascii_to_hex(
            msg_answer + msg_answer_idx, " [running]", rl_strlen(" [running]"));
    } else {
        msg_answer_idx +=
            rsp_ascii_to_hex(msg_answer + msg_answer_idx, " [lotto-wait]",
                             rl_strlen(" [lotto-wait]"));
    }

    msg_answer_len = msg_answer_idx;

    msg_answer[msg_answer_len] = '\0';

    gdb_send_msg(fd, (uint8_t *)msg_answer, msg_answer_len);

    return 0;
}

static int64_t
gdb_srv_handle_attached(int fd, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(msg_len > 0);

    uint64_t pid = 0;

    sscanf((char *)msg, "%ld", &pid);

    gdb_send_ack(fd);
    gdb_send_one(fd);

    return 0;
}

static int64_t
gdb_srv_send_currend_thread_id(int fd)
{
    // uint64_t dummy_tid = get_task_id() - 1;
    uint64_t tid = gdb_srv_get_cur_gdb_task();
    char msg_tid[8192];
    uint64_t msg_idx = 0;
    uint64_t msg_len = 0;

    msg_tid[0] = 'Q';
    msg_tid[1] = 'C';
    msg_tid[2] = ' ';
    msg_idx += 3;

    msg_idx += rl_snprintf(msg_tid + msg_idx, 100, "%ld", tid);

    msg_len = msg_idx;

    // logger_infof("sending QC tid message:\n%s\n", msg_tid);

    gdb_send_ack(fd);
    gdb_send_msg(fd, (uint8_t *)msg_tid, msg_len);

    return 0;
}

int64_t
gdb_srv_handle_q(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint64_t msg_idx = 0;
    ASSERT(msg);
    ASSERT(msg[0] == 'q');

    msg_idx++;

    // Supported message
    if (msg_len - msg_idx > rl_strlen("Supported:") &&
        rl_strncmp((char *)msg + msg_idx,
                   "Supported:", rl_strlen("Supported:")) == 0) {
        return gdb_srv_handle_supported(
            fd, msg + msg_idx + rl_strlen("Supported:"),
            msg_len - msg_idx - rl_strlen("Supported:"));
    }

    // Xfer:features:read:target.xml:0,ffb
    if (msg_len - msg_idx > rl_strlen("Xfer:") &&
        rl_strncmp((char *)msg + msg_idx, "Xfer:", rl_strlen("Xfer:")) == 0) {
        return gdb_srv_handle_xfer(fd, msg + msg_idx + rl_strlen("Xfer:"),
                                   msg_len - msg_idx - rl_strlen("Xfer:"));
    }

    if (msg_len - msg_idx == rl_strlen("TStatus") &&
        strcmp((char *)msg + msg_idx, "TStatus") == 0) {
        gdb_send_ack(fd);
        gdb_send_empty(fd);
        return 0;
    }

    if (msg_len - msg_idx == rl_strlen("Offsets") &&
        strcmp((char *)msg + msg_idx, "Offsets") == 0) {
        gdb_send_ack(fd);
        gdb_send_empty(fd);
        return 0;
    }

    if (msg_len - msg_idx == rl_strlen("Symbol::") &&
        strcmp((char *)msg + msg_idx, "Symbol::") == 0) {
        gdb_send_ack(fd);
        gdb_send_empty(fd);
        return 0;
    }

    if (msg_len - msg_idx >= rl_strlen("xThreadInfo") &&
        rl_strncmp((char *)msg + msg_idx + 1, "ThreadInfo",
                   rl_strlen("ThreadInfo")) == 0) {
        return gdb_srv_handle_thread_info(fd, msg + msg_idx, msg_len - msg_idx);
    }

    if (msg_len - msg_idx >= rl_strlen("ThreadExtraInfo,") &&
        rl_strncmp((char *)msg + msg_idx, "ThreadExtraInfo,",
                   rl_strlen("ThreadExtraInfo,")) == 0) {
        return gdb_srv_handle_thread_extra_info(fd, msg + msg_idx,
                                                msg_len - msg_idx);
    }

    if (msg_len - msg_idx == rl_strlen("C") &&
        strcmp((char *)msg + msg_idx, "C") == 0) {
        return gdb_srv_send_currend_thread_id(fd);
    }

    if (msg_len - msg_idx > rl_strlen("Attached:") &&
        rl_strncmp((char *)msg + msg_idx,
                   "Attached:", rl_strlen("Attached:")) == 0) {
        return gdb_srv_handle_attached(
            fd, msg + msg_idx + rl_strlen("Attached:"),
            msg_len - msg_idx - rl_strlen("Attached:"));
    }

    if (msg_len - msg_idx >= rl_strlen("Rcmd") &&
        rl_strncmp((char *)msg + msg_idx, "Rcmd", rl_strlen("Rcmd")) == 0) {
        return gdb_srv_handle_Rcmd(fd, msg + msg_idx, msg_len - msg_idx);
    }

    logger_infof("Cannot handle q message:\n%s\n", msg);
    ASSERT(0 && "Unsupported q message");

    return 0;
}

int64_t
gdb_srv_send_T_info(int fd, uint32_t halted_sig)
{
    uint8_t msg_answer[GDB_MAX_MESSAGE_LENGTH];
    uint64_t msg_answer_idx = 0;
    uint64_t msg_answer_len = 0;

    uint32_t signal_halt_reason = halted_sig;
    uint64_t current_pid        = 1;
    uint64_t current_tid        = gdb_get_execution_halted_gdb_tid();

    msg_answer[msg_answer_idx] = 'T';
    msg_answer_idx++;

    // give signal num on why it was halted
    msg_answer_idx += rl_snprintf((char *)msg_answer + msg_answer_idx, 16,
                                  "%02d", signal_halt_reason);

    ASSERT(current_tid > 0);
    // put the current thread
    msg_answer_idx +=
        rl_snprintf((char *)msg_answer + msg_answer_idx, 64,
                    "thread:p%02lu.%02lu;", current_pid, current_tid);

    // If signal 5, put stop reason
    // https://sourceware.org/gdb/current/onlinedocs/gdb.html/Stop-Reply-Packets.html
    if (signal_halt_reason == 5 && gdb_has_stop_reason()) {
        msg_answer_idx += rl_snprintf((char *)msg_answer + msg_answer_idx, 64,
                                      "%s:", gdb_get_stop_reason_str());

        // If watchpoint hit, also put data address
        if (gdb_get_stop_reason() == STOP_REASON_awatch ||
            gdb_get_stop_reason() == STOP_REASON_watch ||
            gdb_get_stop_reason() == STOP_REASON_rwatch) {
            msg_answer_idx +=
                rl_snprintf((char *)msg_answer + msg_answer_idx, 64, "%016lu",
                            gdb_get_stop_reason_n());
        }

        msg_answer_idx +=
            rl_snprintf((char *)msg_answer + msg_answer_idx, 64, ";");
    }

    msg_answer_len = msg_answer_idx;

    msg_answer[msg_answer_len] = '\0';

    return gdb_send_msg(fd, msg_answer, msg_answer_len);
}

// give reason why target is halted
int64_t
gdb_srv_handle_questionmark(int fd, uint8_t *msg, uint64_t msg_len)
{
    gdb_send_ack(fd);
    return gdb_srv_send_T_info(fd, 5);
}

// Is Thread alive?
int64_t
gdb_srv_handle_T(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint64_t msg_idx = 0;
    int64_t pid      = 0;
    int64_t tid      = 0;

    ASSERT(msg_len >= 5);
    ASSERT(msg[0] == 'T');
    ASSERT(msg[1] == 'p');

    msg_idx = 2;

    rsp_extract_thread_id((char *)msg + msg_idx, msg_len - msg_idx, &pid, &tid);

    gdb_send_ack(fd);
    gdb_send_ok(fd);

    return 0;
}
