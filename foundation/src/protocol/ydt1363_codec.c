#include "ydt1363_codec.h"

static const char kHexDigits[] = "0123456789ABCDEF";

static int hex_nibble(uint8_t c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static void put_hex(uint8_t *p, uint8_t value) {
    p[0] = (uint8_t)kHexDigits[value >> 4];
    p[1] = (uint8_t)kHexDigits[value & 0x0F];
}

/* LCHKSUM：LENID 三个半字节之和取反加一，只保留 4 位 */
static uint8_t lchksum_of(uint16_t lenid) {
    uint8_t sum = (uint8_t)(((lenid >> 8) & 0x0F) + ((lenid >> 4) & 0x0F) + (lenid & 0x0F));
    return (uint8_t)((~sum + 1) & 0x0F);
}

/* CHKSUM 覆盖 SOI 之后、CHKSUM 之前的全部 ASCII 字符，不含 SOI、CHKSUM 自身和 EOI */
static uint16_t chksum_of(const uint8_t *ascii, size_t len) {
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum = (uint16_t)(sum + ascii[i]);
    }
    return (uint16_t)(~sum + 1);
}

/* LENGTH 的 4 个 ASCII 字符：高半字节是 LCHKSUM，其余 12 位是 LENID */
static uint16_t length_value_of(const uint8_t *p) {
    return (uint16_t)(((uint16_t)hex_nibble(p[0]) << 12) | ((uint16_t)hex_nibble(p[1]) << 8) |
                      ((uint16_t)hex_nibble(p[2]) << 4) | (uint16_t)hex_nibble(p[3]));
}

int ydt1363_encode(uint8_t ver, uint8_t adr, uint8_t cid1, uint8_t cid2, const uint8_t *info,
                   size_t info_len, net_buf_t *out) {
    if (!out || (!info && info_len > 0)) return YDT1363_ERR_ARG;
    if (info_len > YDT1363_MAX_INFO) return YDT1363_ERR_SIZE;

    size_t total = 18 + 2 * info_len;
    if (net_buf_tailroom(out) < total) return YDT1363_ERR_SIZE;

    uint16_t lenid = (uint16_t)(info_len * 2);
    uint8_t len_hi = (uint8_t)(((lenid >> 8) & 0x0F) | (lchksum_of(lenid) << 4));
    uint8_t len_lo = (uint8_t)(lenid & 0xFF);

    uint8_t *p = net_buf_add(out, total);
    p[0] = YDT1363_SOI;
    put_hex(&p[1], ver);
    put_hex(&p[3], adr);
    put_hex(&p[5], cid1);
    put_hex(&p[7], cid2);
    put_hex(&p[9], len_hi);
    put_hex(&p[11], len_lo);
    for (size_t i = 0; i < info_len; i++) {
        put_hex(&p[13 + 2 * i], info[i]);
    }

    uint16_t chksum = chksum_of(&p[1], 12 + 2 * info_len);
    put_hex(&p[13 + 2 * info_len], (uint8_t)(chksum >> 8));
    put_hex(&p[15 + 2 * info_len], (uint8_t)chksum);
    p[17 + 2 * info_len] = YDT1363_EOI;
    return YDT1363_OK;
}

int ydt1363_decode(const uint8_t *frame, size_t len, uint8_t *out_info, size_t cap,
                   size_t *info_len, uint8_t *rtn) {
    if (!frame || !info_len || !rtn) return YDT1363_ERR_ARG;
    *info_len = 0;
    *rtn = 0;

    if (len < 18 || frame[0] != YDT1363_SOI || frame[len - 1] != YDT1363_EOI)
        return YDT1363_ERR_FORMAT;
    /* 所有字段字节都必须是 ASCII 十六进制：坏字符不放过，不做 0xFF 兜底 */
    for (size_t i = 1; i + 1 < len; i++) {
        if (hex_nibble(frame[i]) < 0) return YDT1363_ERR_FORMAT;
    }

    uint16_t length_value = length_value_of(&frame[9]);
    uint16_t lenid = (uint16_t)(length_value & 0x0FFF);
    if (((length_value >> 12) & 0x0F) != lchksum_of(lenid)) return YDT1363_ERR_CHECKSUM;
    if ((lenid & 1u) != 0) return YDT1363_ERR_FORMAT; /* INFO 按整字节成对编码 */

    size_t n = lenid / 2;
    if (len != 18 + 2 * n) return YDT1363_ERR_FORMAT; /* 声明长度与实际缓冲必须一致 */

    uint16_t chksum = length_value_of(&frame[len - 5]);
    if (chksum != chksum_of(&frame[1], len - 6)) return YDT1363_ERR_CHECKSUM;

    uint8_t rtn_value = (uint8_t)((hex_nibble(frame[7]) << 4) | hex_nibble(frame[8]));
    *rtn = rtn_value;
    if (rtn_value != YDT1363_RTN_NORMAL) return YDT1363_ERR_RTN;

    if (n > cap) return YDT1363_ERR_SIZE;
    if (n > 0 && !out_info) return YDT1363_ERR_ARG;
    for (size_t i = 0; i < n; i++) {
        out_info[i] =
            (uint8_t)((hex_nibble(frame[13 + 2 * i]) << 4) | hex_nibble(frame[14 + 2 * i]));
    }
    *info_len = n;
    return YDT1363_OK;
}

int ydt1363_frame_len(const uint8_t *buf, size_t len) {
    if (!buf) return YDT1363_ERR_ARG;
    if (len < 13) return 0;
    if (buf[0] != YDT1363_SOI) return YDT1363_ERR_FORMAT;
    for (size_t i = 1; i < 13; i++) {
        if (hex_nibble(buf[i]) < 0) return YDT1363_ERR_FORMAT;
    }

    uint16_t length_value = length_value_of(&buf[9]);
    uint16_t lenid = (uint16_t)(length_value & 0x0FFF);
    if (((length_value >> 12) & 0x0F) != lchksum_of(lenid)) return YDT1363_ERR_CHECKSUM;
    if ((lenid & 1u) != 0) return YDT1363_ERR_FORMAT;
    return (int)(18 + lenid);
}
