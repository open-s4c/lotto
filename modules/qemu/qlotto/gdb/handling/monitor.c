#include <string.h>

#include <lotto/engine/sequencer.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/handling/execute.h>
#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/qlotto/gdb/util.h>
#include <lotto/unsafe/_sys.h>

#define LOTTO_MAGIC "qlotto:"

static int64_t
gdb_srv_handle_Rcmd_lotto(int fd, char *cmd, uint64_t cmd_len)
{
    ASSERT(cmd_len > 0);

    // rl_fprintf(stderr, "Rcmd lotto got: %s\n", cmd);

    if (strcmp(cmd, "hook-stepi") == 0) {
        gdb_server_set_hook_si();
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    if (strcmp(cmd, "hookpost-stepi") == 0) {
        gdb_server_reset_hook_si();
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    if (strcmp(cmd, "gdb-clock") == 0) {
        char answer[8192];
        uint8_t msg_answer[8192];
        uint64_t msg_answer_idx = 0;
        uint64_t msg_answer_len = 0;
        rl_snprintf(answer, 8192, "%lu", gdb_get_tick());

        msg_answer_idx =
            rsp_ascii_to_hex((char *)msg_answer, answer, rl_strlen(answer));
        msg_answer[msg_answer_idx] = '\0';

        msg_answer_len = msg_answer_idx;

        gdb_send_ack(fd);
        gdb_send_msg(fd, (uint8_t *)msg_answer, msg_answer_len);
        return 0;
    }

    if (strcmp(cmd, "lotto-clock") == 0) {
        char answer[8192];
        uint8_t msg_answer[8192];
        uint64_t msg_answer_idx = 0;
        uint64_t msg_answer_len = 0;
        rl_snprintf(answer, 8192, "%lu", sequencer_get_clk());

        msg_answer_idx =
            rsp_ascii_to_hex((char *)msg_answer, answer, rl_strlen(answer));
        msg_answer[msg_answer_idx] = '\0';

        msg_answer_len = msg_answer_idx;

        gdb_send_ack(fd);
        gdb_send_msg(fd, (uint8_t *)msg_answer, msg_answer_len);
        return 0;
    }

    if (rl_strncmp(cmd, "break-gdb-clock:", rl_strlen("break-gdb-clock:")) ==
        0) {
        uint64_t clk = strtoull(cmd + rl_strlen("break-gdb-clock:"), NULL, 10);
        // rl_fprintf(stderr, "Adding gdb clock breakpoint: %lu\n", clk);
        gdb_add_breakpoint_gdb_clock(clk);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    if (rl_strncmp(
            cmd, "break-lotto-clock:", rl_strlen("break-lotto-clock:")) == 0) {
        uint64_t clk =
            strtoull(cmd + rl_strlen("break-lotto-clock:"), NULL, 10);
        // rl_fprintf(stderr, "Adding lotto clock breakpoint: %lu\n", clk);
        gdb_add_breakpoint_lotto_clock(clk);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    rl_fprintf(stderr, "Rcmd for qlotto not suppoerted: %s\n", cmd);
    ASSERT(0 && "Unsupported monitor Rcmd for qlotto");
    return -1;
}

static int64_t
gdb_srv_handle_Rcmd_generic(int fd, char *cmd, uint64_t cmd_len)
{
    gdb_send_ack(fd);
    gdb_send_error(fd, 22);
    return -1;
}

int64_t
gdb_srv_handle_Rcmd(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint64_t msg_idx = 0;
    uint64_t cmd_len = 0;
    uint64_t cmd_idx = 0;
    char cmd[2048];

    if (rl_strncmp((char *)msg + msg_idx, "Rcmd,", 5) != 0) {
        gdb_send_ack(fd);
        gdb_send_error(fd, 22);
        return -1;
    }
    msg_idx += 5;

    cmd_len = rsp_hex_to_ascii(cmd, (char *)msg + msg_idx, msg_len - msg_idx);
    ASSERT(cmd_len < 2048);
    cmd[cmd_len] = '\0';

    if (rl_strncmp(cmd, LOTTO_MAGIC, rl_strlen(LOTTO_MAGIC)) == 0) {
        cmd_idx += rl_strlen(LOTTO_MAGIC);
        return gdb_srv_handle_Rcmd_lotto(fd, cmd + cmd_idx, cmd_len - cmd_idx);
    }

    return gdb_srv_handle_Rcmd_generic(fd, cmd, cmd_len);
}
