
#include <stdint.h>

#include <lotto/base/map.h>
#include <lotto/qlotto/gdb/gdb_send.h>
#include <lotto/qlotto/gdb/handling/break.h>
#include <lotto/qlotto/gdb/rsp_util.h>

struct gdb_break {
    map_t breakpoints;
    map_t breakpoints_range_exclude;
    map_t watchpoints;
    map_t breakpoints_gdb_clock;
    map_t breakpoints_lotto_clock;
    int64_t gdb_break_next;
} _state_gdb_break;

void
gdb_set_break_next(int64_t next_on_core)
{
    _state_gdb_break.gdb_break_next = next_on_core;
    // rl_fprintf(stderr, "Set next core to break on: %ld\n", next_on_core);
}

int64_t
gdb_get_break_next(void)
{
    return _state_gdb_break.gdb_break_next;
}

void
gdb_add_breakpoint_range_exclude(uint64_t b_pc, uint64_t size, int32_t cpuindex)
{
    event_bp_range_t *bp_cur = (event_bp_range_t *)map_find(
        &_state_gdb_break.breakpoints_range_exclude, b_pc);

    if (NULL != bp_cur) {
        bp_cur->size     = size;
        bp_cur->cpuindex = cpuindex;
        return;
    }

    map_register(&_state_gdb_break.breakpoints_range_exclude, b_pc);
    bp_cur = (event_bp_range_t *)map_find(
        &_state_gdb_break.breakpoints_range_exclude, b_pc);
    ASSERT(NULL != bp_cur);
    bp_cur->size     = size;
    bp_cur->cpuindex = cpuindex;
    return;
    // rl_fprintf(stderr, "BP added 0x%lx\n", b_pc);
}

void
gdb_del_breakpoint_range_exclude(uint64_t b_pc)
{
    event_bp_range_t *bp_cur = (event_bp_range_t *)map_iterate(
        &_state_gdb_break.breakpoints_range_exclude);
    for (; bp_cur; bp_cur = (event_bp_range_t *)map_next((mapitem_t *)bp_cur)) {
        if (b_pc < bp_cur->ti.key || b_pc > (bp_cur->ti.key + bp_cur->size)) {
            map_deregister(&_state_gdb_break.breakpoints_range_exclude,
                           bp_cur->ti.key);
        }
    }
}

// returns false if breakpoint range is set and we are inside that range
// or if there is no range set
bool
gdb_is_outside_breakpoint_range(uint64_t b_pc, int32_t cpuindex)
{
    event_bp_range_t *bp_cur = (event_bp_range_t *)map_iterate(
        &_state_gdb_break.breakpoints_range_exclude);
    for (; bp_cur; bp_cur = (event_bp_range_t *)map_next((mapitem_t *)bp_cur)) {
        if (bp_cur->cpuindex == cpuindex &&
            (b_pc < bp_cur->ti.key || b_pc > bp_cur->ti.key + bp_cur->size)) {
            return true;
        }
    }
    return false;
}

void
gdb_add_breakpoint_gdb_clock(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_find(&_state_gdb_break.breakpoints_gdb_clock, b_pc);
    if (NULL != bp_cur)
        return;

    map_register(&_state_gdb_break.breakpoints_gdb_clock, b_pc);
    // rl_fprintf(stderr, "BP gdb clock added %lu\n", b_pc);
}

bool
gdb_has_breakpoint_gdb_clock(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_iterate(&_state_gdb_break.breakpoints_gdb_clock);
    for (; bp_cur; bp_cur = (event_bp_t *)map_next((mapitem_t *)bp_cur)) {
        if (bp_cur->ti.key <= b_pc) {
            map_deregister(&_state_gdb_break.breakpoints_gdb_clock,
                           bp_cur->ti.key);
            return true;
        }
    }
    return false;
}

void
gdb_add_breakpoint_lotto_clock(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_find(&_state_gdb_break.breakpoints_lotto_clock, b_pc);
    if (NULL != bp_cur)
        return;

    map_register(&_state_gdb_break.breakpoints_lotto_clock, b_pc);
    // rl_fprintf(stderr, "BP added 0x%lx\n", b_pc);
}

bool
gdb_has_breakpoint_lotto_clock(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_iterate(&_state_gdb_break.breakpoints_lotto_clock);
    for (; bp_cur; bp_cur = (event_bp_t *)map_next((mapitem_t *)bp_cur)) {
        if (bp_cur->ti.key <= b_pc) {
            map_deregister(&_state_gdb_break.breakpoints_lotto_clock,
                           bp_cur->ti.key);
            return true;
        }
    }
    return false;
}

void
gdb_add_breakpoint(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_find(&_state_gdb_break.breakpoints, b_pc);
    if (NULL != bp_cur)
        return;

    map_register(&_state_gdb_break.breakpoints, b_pc);
    // rl_fprintf(stderr, "BP added 0x%lx\n", b_pc);
}

void
gdb_del_breakpoint(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_find(&_state_gdb_break.breakpoints, b_pc);
    if (NULL == bp_cur)
        return;

    map_deregister(&_state_gdb_break.breakpoints, b_pc);
}

bool
gdb_has_breakpoint(uint64_t b_pc)
{
    event_bp_t *bp_cur =
        (event_bp_t *)map_find(&_state_gdb_break.breakpoints, b_pc);
    if (NULL == bp_cur)
        return false;

    return true;
}

// currently not used due to instruction exact breaking.
bool
gdb_has_breakpoint_in_range(tb_info_t *tb_info)
{
    uint64_t tb_pc_start = tb_info->tb_pc;
    uint64_t tb_pc_end   = tb_info->tb_pc + 4 * tb_info->tb_n_insns -
                         4; // last insn pc for aarch64

    event_bp_t *bp_cur =
        (event_bp_t *)map_iterate(&_state_gdb_break.breakpoints);
    for (; bp_cur; bp_cur = (event_bp_t *)map_next((mapitem_t *)bp_cur)) {
        if (bp_cur->ti.key >= tb_pc_start &&
            bp_cur->ti.key <= tb_pc_end) { // found breakpoint
            log_infof("BP 0x%lx in range [0x%lx,0x%lx]!\n", bp_cur->ti.key,
                      tb_pc_start, tb_pc_end);
            return true;
        }
    }
    return false;
}

void
gdb_add_watchpoint(uint64_t addr, uint64_t size, gdb_wp_type_t type)
{
    event_wp_t *wp_cur =
        (event_wp_t *)map_find(&_state_gdb_break.watchpoints, addr);
    if (NULL != wp_cur)
        return;

    wp_cur = (event_wp_t *)map_register(&_state_gdb_break.watchpoints, addr);
    wp_cur->size = size;
    wp_cur->type = type;
    // rl_fprintf(stderr, "WP added 0x%lx\n", b_pc);
}

void
gdb_del_watchpoint(uint64_t addr, uint64_t size, gdb_wp_type_t type)
{
    event_wp_t *wp_cur =
        (event_wp_t *)map_find(&_state_gdb_break.watchpoints, addr);
    if (NULL == wp_cur)
        return;

    map_deregister(&_state_gdb_break.watchpoints, addr);
}

bool
gdb_has_watchpoint(uint64_t addr, bool is_write)
{
    event_wp_t *wp_cur =
        (event_wp_t *)map_iterate(&_state_gdb_break.watchpoints);
    for (; wp_cur; wp_cur = (event_wp_t *)map_next((mapitem_t *)wp_cur)) {
        if (addr >= wp_cur->ti.key && addr < wp_cur->ti.key + wp_cur->size) {
            // found possible breakpoint
            if (wp_cur->type == gdb_wp_access)
                return true;

            if (wp_cur->type == gdb_wp_write && is_write)
                return true;

            if (wp_cur->type == gdb_wp_read && !is_write)
                return true;
        }
    }
    return false;
}

// delete breakpoint
int64_t
gdb_srv_handle_z(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint64_t msg_idx = 0;

    ASSERT(msg_len >= 5);
    ASSERT(msg[0] == 'z');
    ASSERT(msg[1] >= '0' && msg[1] <= '4');

    if (msg[1] == '0' || msg[1] == '1') {
        // sw or hw break point
        ASSERT(msg[2] == ',');

        msg_idx = 3;

        char *sep_next    = strchr((char *)msg + msg_idx, ',');
        uint64_t addr_len = sep_next - (char *)msg - msg_idx;
        ASSERT(sep_next != NULL);

        uint64_t b_pc = rsp_str_to_hex((char *)msg + msg_idx, addr_len);

        // log_infof("Got request to delete breakpoint at pc 0x%lx\n", b_pc);
        gdb_del_breakpoint(b_pc);
        gdb_send_ack(fd);
        gdb_send_ok(fd);

        return 0;
    }

    ASSERT(msg[2] == ',');
    msg_idx        = 3;
    char *next_sep = strchr((char *)msg + msg_idx, ',');
    ASSERT(next_sep != NULL);
    uint64_t addr_len = next_sep - (char *)msg - msg_idx;
    uint64_t wp_addr  = rsp_str_to_hex((char *)msg + msg_idx, addr_len);
    msg_idx += addr_len;
    ASSERT(msg[msg_idx] == ',');
    msg_idx++;
    uint64_t wp_size = rsp_str_to_hex((char *)msg + msg_idx, msg_len - msg_idx);

    if (msg[1] == '2') {
        // hardware _write_ watchpoint
        gdb_del_watchpoint(wp_addr, wp_size, gdb_wp_write);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }
    if (msg[1] == '3') {
        // hardware _read_ watchpoint
        gdb_del_watchpoint(wp_addr, wp_size, gdb_wp_read);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    if (msg[1] == '4') {
        // hardware _access_ watchpoint
        gdb_del_watchpoint(wp_addr, wp_size, gdb_wp_access);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    return -1;
}

// add breakpoint
int64_t
gdb_srv_handle_Z(int fd, uint8_t *msg, uint64_t msg_len)
{
    uint64_t msg_idx = 0;

    ASSERT(msg_len >= 5);
    ASSERT(msg[0] == 'Z');
    ASSERT(msg[1] >= '0' && msg[1] <= '4');

    if (msg[1] == '0' || msg[1] == '1') {
        // sw or hw break point
        ASSERT(msg[2] == ',');
        msg_idx = 3;

        char *sep_next    = strchr((char *)msg + msg_idx, ',');
        uint64_t addr_len = sep_next - (char *)msg - msg_idx;
        ASSERT(sep_next != NULL);

        uint64_t b_pc = rsp_str_to_hex((char *)msg + msg_idx, addr_len);

        // log_printf("Got request to add breakpoint at addr 0x%lx\n", b_pc);
        gdb_add_breakpoint(b_pc);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    ASSERT(msg[2] == ',');
    msg_idx        = 3;
    char *next_sep = strchr((char *)msg + msg_idx, ',');
    ASSERT(next_sep != NULL);
    uint64_t addr_len = next_sep - (char *)msg - msg_idx;
    uint64_t wp_addr  = rsp_str_to_hex((char *)msg + msg_idx, addr_len);
    msg_idx += addr_len;
    ASSERT(msg[msg_idx] == ',');
    msg_idx++;
    uint64_t wp_size = rsp_str_to_hex((char *)msg + msg_idx, msg_len - msg_idx);

    if (msg[1] == '2') {
        // hardware _write_ watchpoint
        gdb_add_watchpoint(wp_addr, wp_size, gdb_wp_write);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }
    if (msg[1] == '3') {
        // hardware _read_ watchpoint
        gdb_add_watchpoint(wp_addr, wp_size, gdb_wp_read);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    if (msg[1] == '4') {
        // hardware _access_ watchpoint
        gdb_add_watchpoint(wp_addr, wp_size, gdb_wp_access);
        gdb_send_ack(fd);
        gdb_send_ok(fd);
        return 0;
    }

    return -1;
}

void
gdb_break_init(void)
{
    map_init(&_state_gdb_break.breakpoints,
             MARSHABLE_STATIC(sizeof(event_bp_t)));
    map_init(&_state_gdb_break.breakpoints_range_exclude,
             MARSHABLE_STATIC(sizeof(event_bp_range_t)));
    map_init(&_state_gdb_break.breakpoints_gdb_clock,
             MARSHABLE_STATIC(sizeof(event_bp_t)));
    map_init(&_state_gdb_break.breakpoints_lotto_clock,
             MARSHABLE_STATIC(sizeof(event_bp_t)));
    map_init(&_state_gdb_break.watchpoints,
             MARSHABLE_STATIC(sizeof(event_wp_t)));
    _state_gdb_break.gdb_break_next = GDB_BREAK_STEP_OFF;
}
