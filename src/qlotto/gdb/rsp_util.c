/*
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <lotto/qlotto/gdb/rsp_util.h>
#include <lotto/unsafe/_sys.h>
#include <lotto/util/casts.h>

/**
 * Converts a 4-bit value to ascii 0-f
 */
uint8_t
rsp_hex_to_chr(uint8_t hex)
{
    if (hex > 15)
        return 0;

    if (hex <= 9)
        return '0' + hex;

    if (hex >= 10)
        return 'a' - 10 + hex;

    ASSERT(0 && "Invalid hex to character conversion");
    return 0;
}


/**
 * Converts an ascii character 0-f to a 4-bit value
 */
uint8_t
rsp_chr_to_hex(uint8_t chr)
{
    if (chr >= '0' && chr <= '9') {
        // logger_infof( "rsp_chr_to_hex 1 %c to %x\n", chr, chr - '0');
        return chr - '0';
    }

    if (chr >= 'a' && chr <= 'f') {
        // logger_infof( "rsp_chr_to_hex 2 %c to %x\n", chr, chr + 10 - 'a');
        return chr + 10 - 'a';
    }

    ASSERT(0 && "character out of range for chr_to_hex conversion.");
    return 255;
}


/**
 * Converts a 64-bit value into a string of 0-f characters
 * returns number of characters put into the string
 * if num_chars_to_use > 0, it adds leading 00 until
 * 'num_chars_to_use' characters are used
 */
uint64_t
rsp_hex_to_str(char *str, uint64_t hex, uint64_t num_chars_to_use)
{
    uint64_t bit_length = 0;
    uint64_t chr_length = 0;

    if (hex == 0)
        bit_length = 4;
    else
        bit_length = 64 - __builtin_clzl(hex);

    chr_length = bit_length / 4;
    if (bit_length % 4 != 0)
        chr_length++;


    if (num_chars_to_use > 0) {
        ASSERT(chr_length <= num_chars_to_use);
        chr_length = num_chars_to_use;
    }

    for (uint64_t idx = chr_length - 1;; idx--) {
        uint8_t chex = hex & 0xF;
        str[idx]     = CAST_TYPE(char, rsp_hex_to_chr(chex));
        hex >>= 4;
        if (idx == 0)
            break;
    }

    // TODO: remove!
    //  str[chr_length] = '\0';
    //  logger_infof( "Coverted 0x%lx to %s\n", hex, str);

    ASSERT(hex == 0);

    return chr_length;
}

/**
 * Converts an ascii character string out of 0-f to a 64-bit value
 */
uint64_t
rsp_str_to_hex(char *str, uint64_t str_len)
{
    uint64_t hex = 0;

    for (uint64_t idx = 0; idx < str_len; idx++) {
        char chr = str[idx];
        hex <<= 4;
        hex |= rsp_chr_to_hex(chr);
    }

    // logger_infof( "Coverted %s to 0x%lx\n", str, hex);

    return hex;
}

/**
 * Returns length of escaped message or negative value on error
 * uint8_t* msg_escaped; OUT buffer for escaped message
 * uint8_t* chk_sum;     OUT uint8_t for calculated checksum
 * uint8_t* msg;          IN massage to be escaped
 * uint64_t msg_len;      IN length of message
 */
int64_t
rsp_escape_msg(uint8_t *msg_escaped, uint8_t *chk_sum, uint8_t *msg,
               uint64_t msg_len)
{
    ASSERT(msg_escaped != NULL);
    ASSERT(chk_sum != NULL);
    ASSERT(msg != NULL);
    ASSERT(msg_len > 0);

    uint64_t enc_idx = 0;

    for (uint64_t msg_idx = 0; msg_idx < msg_len; msg_idx++) {
        uint8_t chr = msg[msg_idx];

        // check for characters to be escaped
        if (('#' == chr) || ('$' == chr) || ('*' == chr) || ('}' == chr)) {
            // put escape character
            msg_escaped[enc_idx] = (uint8_t)'}';
            enc_idx++;

            // adjust checksum for escape character
            *chk_sum += (uint8_t)'}';

            // mark the actual message character
            chr ^= (uint8_t)0x20;
        }

        // put current character
        msg_escaped[enc_idx] = chr;
        enc_idx++;

        // adjust checksum for character
        *chk_sum += CAST_TYPE(uint8_t, chr);
    }
    msg_escaped[enc_idx] = '\0';
    return CAST_TYPE(int64_t, enc_idx);
}

/**
 * Returns length of unescaped message or negative value on error
 * uint8_t* msg;        OUT buffer for message
 * uint8_t* chk_sum;    OUT uint8_t for calculated checksum
 * uint8_t* escaped_msg; IN massage to be escaped
 * uint64_t msg_len;     IN length of escaped message
 */
int64_t
rsp_unescape_msg(uint8_t *msg, uint8_t *chk_sum, uint8_t *msg_escaped,
                 uint64_t *msg_esc_len)
{
    ASSERT(msg != NULL);
    ASSERT(chk_sum != NULL);
    ASSERT(msg_escaped != NULL);
    ASSERT(msg_esc_len != NULL);
    ASSERT(*msg_esc_len > 0);

    uint64_t msg_idx = 0;
    uint64_t enc_idx = 0;

    for (enc_idx = 0; enc_idx < *msg_esc_len; enc_idx++) {
        uint8_t chr = msg_escaped[enc_idx];

        // check for escape character
        if (('}' == chr)) {
            // skip escape character
            enc_idx++;

            // adjust checksum for escape character
            *chk_sum += (uint8_t)'}';

            // get the escaped character
            chr = msg_escaped[enc_idx];

            // adjust checksum for escaped character
            *chk_sum += CAST_TYPE(uint8_t, chr);

            // unmark the actual message character
            chr ^= (uint8_t)0x20;

            // put unmarked character in message
            msg[msg_idx] = chr;
            msg_idx++;
            continue;
        }

        // character not escaped

        // unscaped message end char, end of message
        if ('#' == chr)
            break;

        // put current character
        msg[msg_idx] = chr;
        msg_idx++;

        // adjust checksum for character
        *chk_sum += CAST_TYPE(uint8_t, chr);
    }
    *msg_esc_len = enc_idx;

    // put null character after message
    msg[msg_idx] = '\0';
    return CAST_TYPE(int64_t, msg_idx);
}

/**
 * Returns length of constructed packet or negative value on error
 * uint8_t* pkt;     OUT buffer for constructed packet
 * uint8_t* msg;      IN massage to be stored in packet
 * uint64_t msg_len;  IN length of message
 */
int64_t
rsp_construct_pkt(uint8_t *pkt, uint8_t *msg, uint64_t msg_len)
{
    ASSERT(pkt != NULL);
    ASSERT(msg != NULL);
    ASSERT(msg_len > 0);

    uint64_t pkt_idx         = 0;
    uint8_t chk_sum          = 0;
    uint64_t msg_escaped_len = 0;

    // put rsp message start
    pkt[0] = (uint8_t)'$';
    pkt_idx++;

    // put escaped message into packet
    msg_escaped_len = rsp_escape_msg(pkt + pkt_idx, &chk_sum, msg, msg_len);
    ASSERT(msg_escaped_len > 0);

    // increase packet index by escaped message length
    pkt_idx += msg_escaped_len;

    // put rsp message end character
    pkt[pkt_idx] = (uint8_t)'#';
    pkt_idx++;

    // put checksum at end
    uint8_t chksum1 = CAST_TYPE(uint8_t, rsp_hex_to_chr(chk_sum >> 4));
    uint8_t chksum2 = CAST_TYPE(uint8_t, rsp_hex_to_chr(chk_sum & 0xF));

    // logger_infof( "Calculated checksum: %c%c\n", chksum1, chksum2);

    pkt[pkt_idx] = chksum1;
    pkt_idx++;
    pkt[pkt_idx] = chksum2;
    pkt_idx++;

    pkt[pkt_idx] = '\0';
    return CAST_TYPE(int64_t, pkt_idx);
}

/**
 * Returns length of extracted message or negative value on error
 * uint8_t* msg;     OUT buffer for extracted message
 * uint8_t* pkt;      IN packet to extract message from
 * uint64_t* pkt_len;  IN&OUT length of packet
 */
int64_t
rsp_extract_pkt(uint8_t *msg, uint8_t *pkt, uint64_t *pkt_len)
{
    ASSERT(msg != NULL);
    ASSERT(pkt != NULL);
    ASSERT(pkt_len != NULL);
    ASSERT(*pkt_len > 0);

    uint64_t pkt_idx           = 0;
    uint8_t chk_sum            = 0;
    uint64_t msg_unescaped_len = 0;

    uint64_t msg_escaped_len = 0;

    // check rsp message start
    ASSERT(pkt[0] == (uint8_t)'$' || pkt[0] == (uint8_t)'%');
    pkt_idx++;

    // extract unescaped message from packet
    // escaped packet length is pkt length - 4 (start, end, chksum1, chksum2)
    msg_escaped_len = (*pkt_len) - 4;

    // check for empty message
    if (rl_strncmp((char *)pkt, "$#00", 4) == 0)
        return 0;

    msg_unescaped_len =
        rsp_unescape_msg(msg, &chk_sum, pkt + pkt_idx, &msg_escaped_len);
    ASSERT(msg_unescaped_len > 0);

    // increase packet index by escaped message length
    pkt_idx += msg_unescaped_len;

    // check rsp message end character
    if (pkt[pkt_idx] != (uint8_t)'#') {
        return PKT_EXTR_INCOMPLETE;
    }

    // ASSERT(pkt[pkt_idx] == (uint8_t)'#');
    pkt_idx++;

    // check checksum at end
    uint8_t chksum1 = CAST_TYPE(uint8_t, rsp_hex_to_chr(chk_sum >> 4));
    uint8_t chksum2 = CAST_TYPE(uint8_t, rsp_hex_to_chr(chk_sum & 0xF));

    ASSERT(pkt[pkt_idx] == chksum1);
    pkt_idx++;

    ASSERT(pkt[pkt_idx] == chksum2);
    pkt_idx++;

    *pkt_len = pkt_idx;

    msg[msg_unescaped_len] = '\0';

    return CAST_TYPE(int64_t, msg_unescaped_len);
}

int64_t
rsp_construct_thread_id(char *msg, int64_t pid, int64_t tid)
{
    ASSERT(msg != NULL);
    uint64_t msg_idx = 0;
    msg_idx += rsp_hex_to_str(msg, pid, 0);
    msg[msg_idx] = '.';
    msg_idx++;

    if (tid == -1) {
        msg[msg_idx] = '-';
        msg_idx++;
        msg[msg_idx] = '1';
        msg_idx++;
        return CAST_TYPE(int64_t, msg_idx);
    }

    msg_idx += rsp_hex_to_str(msg + msg_idx, tid, 0);
    return CAST_TYPE(int64_t, msg_idx);
}

int64_t
rsp_extract_thread_id(char *msg, uint64_t msg_len, int64_t *pid, int64_t *tid)
{
    ASSERT(msg != NULL);
    ASSERT(msg_len >= 3);
    char *dot            = NULL;
    char *semicolon      = NULL;
    char *pid_str        = msg;
    char *tid_str        = NULL;
    uint64_t pid_len     = 0;
    uint64_t tid_len     = 0;
    uint64_t extract_len = msg_len;

    int64_t pid_res = 0;
    int64_t tid_res = 0;

    dot = strchr(msg, '.');
    ASSERT(NULL != dot);

    semicolon = strchr(msg, ';');
    if (NULL != semicolon) {
        extract_len = semicolon - msg;
    }

    pid_len = dot - msg;
    tid_len = extract_len - pid_len - 1;


    ASSERT(pid_len >= 1);
    ASSERT(tid_len >= 1);

    pid_str = msg;
    tid_str = msg + pid_len + 1;

    pid_res = CAST_TYPE(int64_t, rsp_str_to_hex(pid_str, pid_len));
    ASSERT(pid_res <= INT32_MAX);
    if (pid != NULL)
        *pid = pid_res;

    if (tid_str[0] == '-') {
        if (tid != NULL)
            *tid = -1;
        ASSERT(tid_str[1] == '1');
        return CAST_TYPE(int64_t, extract_len);
    }

    tid_res = CAST_TYPE(int64_t, rsp_str_to_hex(tid_str, tid_len));
    ASSERT(tid_res <= INT32_MAX);

    if (tid != NULL) {
        *tid = tid_res;
    }

    return CAST_TYPE(int64_t, extract_len);
}

int64_t
rsp_ascii_to_hex(char *hex, char *ascii, uint64_t ascii_len)
{
    uint64_t ascii_idx = 0;
    uint64_t hex_idx   = 0;
    for (ascii_idx = 0; ascii_idx < ascii_len; ascii_idx++, hex_idx += 2) {
        char chr;
        sscanf(ascii + ascii_idx, "%c", &chr);
        rl_snprintf(hex + hex_idx, 3, "%02hhx", chr);
    }
    return CAST_TYPE(int64_t, hex_idx);
}

int64_t
rsp_hex_to_ascii(char *ascii, char *hex, uint64_t hex_len)
{
    ASSERT((hex_len % 2) == 0);
    uint64_t ascii_idx = 0;
    uint64_t hex_idx   = 0;
    for (hex_idx = 0; hex_idx < hex_len; hex_idx += 2, ascii_idx++) {
        char chr;
        sscanf(hex + hex_idx, "%02hhx", &chr);
        rl_snprintf(ascii + ascii_idx, 2, "%c", chr);
    }

    return CAST_TYPE(int64_t, ascii_idx);
}
