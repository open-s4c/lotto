/*
 */

#ifndef RSP_UTIL_H
#define RSP_UTIL_H

#include <stddef.h>
#include <stdint.h>

enum pkt_extract_e {
    PKT_EXTR_INCOMPLETE = -100,
    PKT_EXTR_ERROR,
};

uint8_t rsp_hex_to_chr(uint8_t hex);
uint8_t rsp_chr_to_hex(uint8_t chr);

uint64_t rsp_hex_to_str(char *str, uint64_t hex, uint64_t num_chars_to_use);
uint64_t rsp_str_to_hex(char *str, uint64_t str_len);

int64_t rsp_construct_pkt(uint8_t *pkt, uint8_t *msg, uint64_t msg_len);
int64_t rsp_extract_pkt(uint8_t *msg, uint8_t *pkt, uint64_t *pkt_len);

int64_t rsp_construct_thread_id(char *msg, int64_t pid, int64_t tid);
int64_t rsp_extract_thread_id(char *msg, uint64_t msg_len, int64_t *pid,
                              int64_t *tid);

int64_t rsp_ascii_to_hex(char *hex, char *ascii, uint64_t ascii_len);
int64_t rsp_hex_to_ascii(char *ascii, char *hex, uint64_t hex_len);

#endif // RSP_UTIL_H
