#ifndef LOTTO_GDB_BREAK_H
#define LOTTO_GDB_BREAK_H

#include <stdint.h>

#include <lotto/base/map.h>

#define GDB_BREAK_ANY      INT32_MIN
#define GDB_BREAK_STEP_OFF -1

typedef enum gdb_wp_type_e {
    gdb_wp_none,
    gdb_wp_write,
    gdb_wp_read,
    gdb_wp_access,
} gdb_wp_type_t;

typedef struct event_wp_s {
    mapitem_t ti;
    uint64_t size;
    gdb_wp_type_t type;
} event_wp_t;

typedef struct event_bp_s {
    mapitem_t ti;
} event_bp_t;

typedef struct event_bp_range_s {
    mapitem_t ti;
    uint64_t size;
    int32_t cpuindex;
} event_bp_range_t;

typedef struct tb_info_s {
    uint64_t tb_pc;
    uint64_t tb_n_insns;
} tb_info_t;

void gdb_break_init(void);

int64_t gdb_srv_handle_z(int fd, uint8_t *msg, uint64_t msg_len);
int64_t gdb_srv_handle_Z(int fd, uint8_t *msg, uint64_t msg_len);

int64_t gdb_get_break_next(void);
void gdb_set_break_next(int64_t next_on_core);

void gdb_add_breakpoint(uint64_t b_pc);
void gdb_del_breakpoint(uint64_t b_pc);
bool gdb_has_breakpoint(uint64_t b_pc);

void gdb_add_watchpoint(uint64_t addr, uint64_t size, gdb_wp_type_t type);
void gdb_del_watchpoint(uint64_t addr, uint64_t size, gdb_wp_type_t type);
bool gdb_has_watchpoint(uint64_t addr, bool is_write);

void gdb_add_breakpoint_range_exclude(uint64_t b_pc, uint64_t size,
                                      int32_t cpuindex);
void gdb_del_breakpoint_range_exclude(uint64_t b_pc);
bool gdb_is_outside_breakpoint_range(uint64_t b_pc, int32_t cpuindex);

void gdb_add_breakpoint_gdb_clock(uint64_t b_pc);
bool gdb_has_breakpoint_gdb_clock(uint64_t b_pc);
void gdb_add_breakpoint_lotto_clock(uint64_t b_pc);
bool gdb_has_breakpoint_lotto_clock(uint64_t b_pc);

#endif // LOTTO_GDB_BREAK_H
